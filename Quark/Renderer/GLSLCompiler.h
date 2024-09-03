#pragma once
#include "Quark/Graphic/Shader.h"

namespace quark {

class CompileOptions {
public:
	CompileOptions() = default;

	void AddDefinitions(const std::vector<std::string>& definitions);
	void AddDefine(const std::string& def);
	void AddUndefine(const std::string& undef);

	const std::string& GetPreamble() const { return m_Preamble; }
	const std::vector<std::string>& GetProcesses() const { return m_Processes; }

private:
	void FixLine(std::string& line);

	std::string m_Preamble;
	std::vector<std::string> m_Processes;

};

/// Helper class to generate SPIRV code from GLSL source
/// Currently only support compiling for one shader stage and vulkan 1.3
class GLSLCompiler {
public:
	enum class Target
	{
		VULKAN_VERSION_1_3,
		VULKAN_VERSION_1_1,
	};

	GLSLCompiler() = default;

	void SetTarget(Target target);

	void SetSource(std::string source, std::string sourcePath, graphic::ShaderStage stage);
	void SetSourceFromFile(const std::string& filePath, graphic::ShaderStage stage);

	bool Compile(std::string& outMessages, std::vector<uint32_t>& outSpirv, const CompileOptions& ops = {});

private:
	std::string m_SourcePath;
	std::string m_Source;

	Target m_Target = Target::VULKAN_VERSION_1_1;

	graphic::ShaderStage m_ShaderStage = graphic::ShaderStage::MAX_ENUM;


};
}