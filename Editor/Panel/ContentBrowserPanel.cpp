#include "Editor/Panel/ContentBrowserPanel.h"

#include <Quark/Core/FileSystem.h>
#include <Quark/Asset/TextureImporter.h>
#include <Quark/UI/UI.h>

namespace quark {

ContentBrowserPanel::ContentBrowserPanel()
{
	TextureImporter loader;
	m_fileIcon = loader.ImportStb("BuiltInResources/Icons/ContentBrowser/FileIcon.png");
	m_folderIcon = loader.ImportStb("BuiltInResources/Icons/ContentBrowser/DirectoryIcon.png");
}

ContentBrowserPanel::ContentBrowserPanel(const std::filesystem::path& basePath)
	: ContentBrowserPanel()
{
	Init(basePath);
}

void ContentBrowserPanel::Init(const std::filesystem::path& basePath)
{
	m_baseDirectory = basePath;
	m_currentDirectory = m_baseDirectory;
}

void ContentBrowserPanel::OnImGuiUpdate()
{
	
	ImGui::Begin("Content Browser");

	if (!m_currentDirectory.empty())
	{
		if (m_currentDirectory != m_baseDirectory)
		{
			if (ImGui::Button("<-"))
			{
				m_currentDirectory = m_currentDirectory.parent_path();
			}
		}


		static float padding = 16.0f;
		static float thumbnailSize = 128.0f;
		float cellSize = thumbnailSize + padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, 0, false);

		for (auto& directoryEntry : std::filesystem::directory_iterator(m_currentDirectory))
		{
			const auto& path = directoryEntry.path();
			std::string filenameString = path.filename().string();

			ImTextureID textureId = directoryEntry.is_directory() ?
				UI::Get()->GetOrCreateTextureId(m_folderIcon) : UI::Get()->GetOrCreateTextureId(m_fileIcon);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton(filenameString.c_str(), textureId, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
			ImGui::PopStyleColor();

			if (ImGui::BeginDragDropSource())
			{
				std::string pathString = path.string();
				ImGui::TextUnformatted(filenameString.c_str());
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathString.c_str(), pathString.size() + 1);
				ImGui::EndDragDropSource();
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (directoryEntry.is_directory())
					m_currentDirectory /= path.filename();

			}

			ImGui::TextWrapped("%s", filenameString.c_str());
			ImGui::NextColumn();

		}

		ImGui::Columns(1);

		ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
		ImGui::SliderFloat("Padding", &padding, 0, 32);
	}

	ImGui::End();
}



}