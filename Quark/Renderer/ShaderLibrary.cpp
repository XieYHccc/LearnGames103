#include "Quark/qkpch.h"
#include "Quark/Renderer/ShaderLibrary.h"
#include "Quark/Core/Application.h"
#include "Quark/Core/Util/Hash.h"
#include "Quark/Asset/Mesh.h"

namespace quark {
uint64_t VariantSignatureKey::GetHash() const
{
	util::Hasher hasher;
	hasher.u32(meshAttributeMask);

	return hasher.get();
}


ShaderProgram::ShaderProgram(ShaderTemplate* compute)
{
	m_Stages[util::ecast(graphic::ShaderStage::STAGE_COMPUTE)] = compute;
}

ShaderProgram::ShaderProgram(ShaderTemplate* vert, ShaderTemplate* frag)
{
	m_Stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)] = vert;
	m_Stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)] = frag;
}

ShaderProgramVariant* ShaderProgram::GetOrCreateVariant(const VariantSignatureKey& key)
{
	uint64_t hash = key.GetHash();

	auto it = m_Variants.find(hash);
	if (it != m_Variants.end())
	{
		return it->second.get();
	}
	else
	{
		ShaderTemplateVariant* vert = m_Stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)]->GetOrCreateVariant(key);
		ShaderTemplateVariant* frag = m_Stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)]->GetOrCreateVariant(key);

		Scope<ShaderProgramVariant> newVariant = CreateScope<ShaderProgramVariant>(vert, frag);

		m_Variants[hash] = std::move(newVariant);

		return m_Variants[hash].get();
	}
	return nullptr;
}

ShaderLibrary::ShaderLibrary()
{
	CORE_LOGI("[ShaderLibrary]: Initialized");
}

ShaderProgram* ShaderLibrary::GetOrCreateGraphicsProgram(const std::string& vert_path, const std::string& frag_path)
{
	util::Hasher h;
	h.string(vert_path);
	h.string(frag_path);
	uint64_t programHash = h.get();

	auto it = m_ShaderPrograms.find(programHash);
	if (it != m_ShaderPrograms.end())
	{
		return it->second.get();
	}
	else // Create ShaderProgram
	{
		ShaderTemplate* vertTemp = GetOrCreateShaderTemplate(vert_path, graphic::ShaderStage::STAGE_VERTEX);
		ShaderTemplate* fragTemp = GetOrCreateShaderTemplate(frag_path, graphic::ShaderStage::STAGE_FRAGEMNT);

		Scope<ShaderProgram> newProgram = CreateScope<ShaderProgram>(vertTemp, fragTemp);

		m_ShaderPrograms[programHash] = std::move(newProgram);

		return m_ShaderPrograms[programHash].get();
	}

}

ShaderProgram* ShaderLibrary::GetOrCreateComputeProgram(const std::string& comp_path)
{
	return nullptr;
}

ShaderTemplate* ShaderLibrary::GetOrCreateShaderTemplate(const std::string& path, graphic::ShaderStage stage)
{
	util::Hasher h;
	h.string(path);

	auto it = m_ShaderTemplates.find(h.get());
	if (it != m_ShaderTemplates.end())
	{
		return it->second.get();
	}
	else
	{
		Scope<ShaderTemplate> newTemplate = CreateScope<ShaderTemplate>(path, stage);
		m_ShaderTemplates[h.get()] = std::move(newTemplate);

		return m_ShaderTemplates[h.get()].get();
	}
}

Ref<graphic::PipeLine> ShaderProgramVariant::GetOrCreatePipeLine(const graphic::PipelineDepthStencilState& ds,
																 const graphic::PipelineColorBlendState& cb, 
																 const graphic::RasterizationState& rs, 
																 const graphic::RenderPassInfo& compatablerp,
																 const graphic::VertexInputLayout& input)
{
	util::Hasher h;

	h.u32(static_cast<uint32_t>(ds.enableDepthTest));
	h.u32(static_cast<uint32_t>(ds.enableDepthWrite));
	h.u32(util::ecast(ds.depthCompareOp));
	h.u32(util::ecast(rs.polygonMode));
	h.u32(util::ecast(rs.cullMode));
	h.u32(util::ecast(rs.frontFaceType));

	CORE_ASSERT(cb.attachments.size() == compatablerp.numColorAttachments)
	for (size_t i = 0; i < compatablerp.numColorAttachments; i++) 
	{
		const auto& att = cb.attachments[i];

		h.u32(static_cast<uint32_t>(att.enable_blend));
		if (att.enable_blend) 
		{
			h.u32(util::ecast(att.colorBlendOp));
			h.u32(util::ecast(att.srcColorBlendFactor));
			h.u32(util::ecast(att.dstColorBlendFactor));
			h.u32(util::ecast(att.alphaBlendOp));
			h.u32(util::ecast(att.srcAlphaBlendFactor));
			h.u32(util::ecast(att.dstAlphaBlendFactor));
		}

		h.u32(util::ecast(compatablerp.colorAttachmentFormats[i]));
	}

	for (const auto& attrib : input.vertexAttribInfos) 
	{
		h.u32(util::ecast(attrib.format));
		h.u32(attrib.offset);
		h.u32(attrib.binding);
	}

	for (const auto& b : input.vertexBindInfos)
	{
		h.u32(b.binding);
		h.u32(b.stride);
		h.u32(util::ecast(b.inputRate));
	}

	uint64_t hash = h.get();

	auto it = m_PipeLines.find(hash);
	if (it != m_PipeLines.end())
	{
		return it->second;
	}
	else 
	{
		graphic::GraphicPipeLineDesc desc = {};
		desc.vertShader = m_Stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)]->gpuShaderHandle;
		desc.fragShader = m_Stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)]->gpuShaderHandle;
		desc.depthStencilState = ds;
		desc.blendState = cb;
		desc.rasterState = rs;
		desc.topologyType = graphic::TopologyType::TRANGLE_LIST;
		desc.renderPassInfo = compatablerp;
		desc.vertexInputLayout = input;

		Ref<graphic::PipeLine> newPipeline = Application::Get().GetGraphicDevice()->CreateGraphicPipeLine(desc);
		m_PipeLines[hash] = newPipeline;

		return newPipeline;
	}
}

ShaderTemplate::ShaderTemplate(const std::string& path, graphic::ShaderStage stage)
	:m_Path(path), m_Stage(stage)
{
	m_Compiler.SetSourceFromFile(path, stage);

	// TODO: Set target based on the renderer
	m_Compiler.SetTarget(GLSLCompiler::Target::VULKAN_VERSION_1_1);
}

ShaderTemplateVariant* ShaderTemplate::GetOrCreateVariant(const VariantSignatureKey &key)
{
	uint64_t hash = key.GetHash();

	auto it = m_Variants.find(hash);
	if (it != m_Variants.end())
	{
		return it->second.get();
	}
	else
	{
		// Compile glsl shader with new key
		GLSLCompiler::CompileOptions ops;
		if (key.meshAttributeMask & MESH_ATTRIBUTE_POSITION_BIT)
			ops.AddDefine("HAVE_POSITION");
		if (key.meshAttributeMask & MESH_ATTRIBUTE_NORMAL_BIT)
			ops.AddDefine("HAVE_NORMAL");
		if (key.meshAttributeMask & MESH_ATTRIBUTE_UV_BIT)
			ops.AddDefine("HAVE_UV");
		if (key.meshAttributeMask & MESH_ATTRIBUTE_VERTEX_COLOR_BIT)
			ops.AddDefine("HAVE_VERTEX_COLOR");

		std::string messages;
		std::vector<uint32_t> spirv;
		if (!m_Compiler.Compile(messages, spirv, ops))
		{
			CORE_LOGE("[ShaderTemplate]: Failed to compile shader: {}: {}", m_Path, messages);
			return nullptr;
		}

		Ref<graphic::Shader> newShader = Application::Get().GetGraphicDevice()->CreateShaderFromBytes(m_Stage, spirv.data(), spirv.size() * sizeof(uint32_t));

		Scope<ShaderTemplateVariant> newVariant = CreateScope<ShaderTemplateVariant>();
		newVariant->gpuShaderHandle = newShader;

		m_Variants[hash] = std::move(newVariant);
		return m_Variants[hash].get();
	}
}

ShaderProgramVariant::ShaderProgramVariant(ShaderTemplateVariant* vert, ShaderTemplateVariant* frag)
{
	m_Stages[util::ecast(graphic::ShaderStage::STAGE_VERTEX)] = vert;
	m_Stages[util::ecast(graphic::ShaderStage::STAGE_FRAGEMNT)] = frag;
}

}