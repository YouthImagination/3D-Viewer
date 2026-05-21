#pragma once

#include "Core/Base.h"
#include "Renderer/VertexArray.h"
#include "Renderer/Material.hpp"
#include "Renderer/Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace viewer
{

	class Mesh {
	public:
		// mesh Data
		std::vector<Vertex>       vertices;
		std::vector<uint> indices;
		MaterialIndex matIndices;

		// constructor
		Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, MaterialIndex matIndex = {})
		{
			this->vertices = vertices;
			this->indices = indices;
			this->matIndices = matIndex;
			m_VAO = CreateRef<VertexArray>();

			// now that we have all the required data, set the vertex buffers and its attribute pointers.
			SetupMesh();
		}

		// render the mesh
		void Draw();

	private:
		// render data 
		Ref<VertexArray> m_VAO;

		// initializes all the buffer objects/arrays
		void SetupMesh();
		void BindTextures();
	};

	class Model
	{
	public:
		// model data 
		RefVector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
		std::vector<Mesh> meshes;
		String directory;
		bool gammaCorrection;
		glm::vec3 Translation = {0.0f, 0.0f, 0.0f};

		// constructor, expects a filepath to a 3D model.
		Model(String const& path, bool gamma = false) : gammaCorrection(gamma)
		{
			LoadModel(path);
		}

		// draws the model, and thus all its meshes
		void Draw();

		glm::mat4 PositionMatrix();

	private:
		// loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
		void LoadModel(std::string const& path);

		// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
		void ProcessNode(aiNode* node, const aiScene* scene);

		Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

		// checks all material textures of a given type and loads the textures if they're not loaded yet.
		// the required info is returned as a Texture struct.
		std::pair<RefVector<Texture>, std::vector<int>> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
	};

}