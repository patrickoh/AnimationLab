#include "Model.h"

Model::Model(glm::vec3 position, glm::mat4 orientation, glm::vec3 scale, const char* file_name, GLuint p_shaderProgramID, bool serialise, bool wireframe)
{
	hasSkeleton = false;

	worldProperties.translation = position;
	worldProperties.orientation = orientation;
	worldProperties.scale = scale;

	glGenVertexArrays (1, &vao);
	vertexCount = 0;

	Load(file_name);

	fileName = file_name;

	shaderProgramID = p_shaderProgramID;

	this->serialise = serialise;

	this->wireframe = wireframe;

	drawMe = true;
	die = false;

	dieTimer = 0.0f;
	dieWaitTime = 0.8f;
}

Model::~Model()
{
	if(hasSkeleton)
		delete skeleton;
}

bool Model::Load(const char* file_name)
{
	const aiScene* scene = aiImportFile (file_name, aiProcess_Triangulate | aiProcess_FlipUVs);
	
	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
	{
		fprintf (stderr, "ERROR: reading mesh %s\n", file_name);
		return false;
	}

	globalInverseTransform = convertAssimpMatrix(scene->mRootNode->mTransformation);
	//glm::mat4 fix = GetModelMatrix() * globalInverseTransform;
	//decomposeTRS(fix, worldProperties.translation, worldProperties.orientation, worldProperties.scale);
	
	printf("LOADING MODEL...\n");
	printf("%i animations\n", scene->mNumAnimations);
	printf("%i cameras\n", scene->mNumCameras);
	printf("%i lights\n", scene->mNumLights);
	printf("%i materials\n", scene->mNumMaterials);
	printf("%i textures\n", scene->mNumTextures);
	printf("%i meshes\n", scene->mNumMeshes);
	
	glBindVertexArray (vao);
	GLuint *buffers = new GLuint [NUM_VBs];
	
	//1. Grab all the data from the submeshes
	vector<glm::vec3> positions;
	vector<glm::vec3> normals;
	vector<glm::vec2> texcoords;

	bool modelHasBones = false;

	int indexCount = 0;

	for(int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++)
	{
		aiMesh* mesh = scene->mMeshes[meshIdx];
		printf("LOADING MESH[%i]\n", meshIdx);
		printf("    %i vertices\n", mesh->mNumVertices);
		printf("    %i UV components\n", mesh->mNumUVComponents);
		printf("    %i anim meshes\n", mesh->mNumAnimMeshes);
		printf("    %i bones\n", mesh->mNumBones);
		printf("    %i faces\n", mesh->mNumFaces);
		printf("	%i normals\n", mesh->HasNormals());

		MeshEntry meshEntry;
		meshEntry.BaseIndex = indexCount;
		meshEntry.BaseVertex = vertexCount;
		meshEntry.NumIndices = mesh->mNumFaces * 3;
		//meshEntry.MaterialIndex = mesh->mMaterialIndex; //This will be used during rendering to bind the proper texture.
		

		vertexCount += mesh->mNumVertices;
		indexCount += meshEntry.NumIndices;

		for(int vertIdx = 0; vertIdx < mesh->mNumVertices; vertIdx++)
		{
			if (mesh->HasPositions ())
				positions.push_back(glm::vec3(mesh->mVertices[vertIdx].x, mesh->mVertices[vertIdx].y, mesh->mVertices[vertIdx].z)); 
			if (mesh->HasNormals ())
				normals.push_back(glm::vec3(mesh->mNormals[vertIdx].x, mesh->mNormals[vertIdx].y, mesh->mNormals[vertIdx].z));
			if (mesh->HasTextureCoords (0)) 
				texcoords.push_back(glm::vec2(mesh->mTextureCoords[0][vertIdx].x, mesh->mTextureCoords[0][vertIdx].y));
		}

		if(mesh->HasFaces())
		{
			for (int i = 0 ; i < mesh->mNumFaces ; i++) 
			{
				const aiFace& Face = mesh->mFaces[i];
				assert(Face.mNumIndices == 3);
			
				indices.push_back(Face.mIndices[0]);
				indices.push_back(Face.mIndices[1]);
				indices.push_back(Face.mIndices[2]);
			}
		}

		if(mesh->HasBones())
			modelHasBones = true;

		if(mesh->mMaterialIndex >=0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			stringstream ss;
			ss << "\nLoading " << material->GetTextureCount(aiTextureType_DIFFUSE) << " aiTextureType_DIFFUSE";
			ss << "\nLoading " << material->GetTextureCount(aiTextureType_SPECULAR) << " aiTextureType_SPECULAR";
			ss << "\nLoading " << material->GetTextureCount(aiTextureType_NORMALS) << " aiTextureType_NORMALS";
			ss << "\nLoading " << material->GetTextureCount(aiTextureType_OPACITY) << " aiTextureType_OPACITY";
			ss << "\nLoading " << material->GetTextureCount(aiTextureType_DISPLACEMENT) << " aiTextureType_DISPLACEMENT";
			std::cout << ss.str();

			///*vector<Texture> diffuseMaps = */LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			//textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

			for(int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++)
			{
				aiString str;
				material->GetTexture(aiTextureType_DIFFUSE, i, &str);

				GLuint textureID = LoadTexture(str.C_Str());
				textures.push_back(textureID); 

				meshEntry.TextureIndex = textureID;
			}

			//vector<Texture> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			//textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		}

		meshEntries.push_back(meshEntry);	
	}

	//2. BUFFER THE DATA
	glGenBuffers(NUM_VBs, buffers);

	if(positions.size() > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers[POS_VB]);
		glBufferData(GL_ARRAY_BUFFER, /*3 * vertexCount * sizeof (GLfloat)*/sizeof(positions[0]) * positions.size(), &positions[0], GL_STATIC_DRAW);
   
		glEnableVertexAttribArray(POS_VB);
		glVertexAttribPointer(POS_VB, 3, GL_FLOAT, GL_FALSE, 0, 0);  
	}

	if(normals.size() > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffers[NORMAL_VB]);
		glBufferData(GL_ARRAY_BUFFER, 3 * vertexCount * sizeof (GLfloat), &normals[0], GL_STATIC_DRAW);
   
		glEnableVertexAttribArray(NORMAL_VB);
		glVertexAttribPointer(NORMAL_VB, 3, GL_FLOAT, GL_FALSE, 0, 0);  
	}
	
	if (texcoords.size() > 0) 
	{
		glBindBuffer (GL_ARRAY_BUFFER, buffers[TEXCOORD_VB]);
		glBufferData (GL_ARRAY_BUFFER, 2 * vertexCount * sizeof (GLfloat), &texcoords[0], GL_STATIC_DRAW);
		
		glVertexAttribPointer (TEXCOORD_VB, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray (TEXCOORD_VB);
	}

	if (indices.size() > 0)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[INDEX_VB]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), &indices[0], GL_STATIC_DRAW);
	}

	if (modelHasBones)
	{
		//aiMesh* mesh = scene->mMeshes[0];

		skeleton = new Skeleton(this);
		hasSkeleton = true;

		printf ("\nBoneHierarchy\n");

		
		/*if (!*/skeleton->ImportAssimpBoneHierarchy(scene, scene->mRootNode, nullptr, true);//)
			//fprintf (stderr, "\nERROR: could not import node tree from mesh\n");

		//printf ("\n\n");

		//skeleton->PrintAiHeirarchy(scene->mRootNode);

		//for(int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) //For every mesh
		//{
		//	for(int boneIdx = 0; boneIdx < scene->mMeshes[meshIdx]->mNumBones; boneIdx++) //For every bone in said mesh
		//	{
		//		std::cout << "\n";
		//		std::cout << scene->mMeshes[meshIdx]->mBones[boneIdx]->mName.data;
		//		std::cout << "\n";
		//	}
		//}

		//std::string roon = skeleton->root->name;
		//skeleton->PrintHeirarchy(skeleton->root);

		printf("\n\nLoading Weights in to buffers\n");

		stringstream debug;

		#define NUM_WEIGHTS_PER_VERTEX 4

		struct VertexWeight {
			GLuint boneIDs[NUM_WEIGHTS_PER_VERTEX];
			float weights[NUM_WEIGHTS_PER_VERTEX];
		};

		vector<VertexWeight> vertexWeights; 
		vertexWeights.resize(vertexCount);

		for (int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
		{
			for(int boneIdx = 0; boneIdx < scene->mMeshes[meshIndex]->mNumBones; boneIdx++)
			{
				const aiBone* bone = scene->mMeshes[meshIndex]->mBones[boneIdx]; //For every bone in the model
				GLfloat boneID = skeleton->GetBone(bone->mName.C_Str())->id;

				for (int j = 0; j < (int)bone->mNumWeights; j++) //loop through its weights
				{
					int vertID = meshEntries[meshIndex].BaseVertex + bone->mWeights[j].mVertexId;
					float weight = bone->mWeights[j].mWeight;

					for(int k = 0; k < NUM_WEIGHTS_PER_VERTEX; k++)
					{
						if(vertexWeights[vertID].weights[k] == 0.0f)
						{
							
							string name = bone->mName.C_Str();
							vertexWeights[vertID].boneIDs[k] = boneID;
							vertexWeights[vertID].weights[k] = weight;
							break; //made breakpoints here
							//they seem to be the same

							//debug << "VERTEX ID: " << vertID;
							//debug.str(std::string());
						//ooopsie :P haha
							//ok it's not here the problem
							//have a look around, i'ma make tea
							//cool could you bring one more cup for me?

						}
					}
				}
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, buffers[WEIGHT_VB]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexWeights[0]) * vertexWeights.size(), &vertexWeights[0], GL_STATIC_DRAW);
		
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 4, GL_INT, sizeof(VertexWeight), (const GLvoid*)0);

		glEnableVertexAttribArray(4);    
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(VertexWeight), (const GLvoid*)offsetof(VertexWeight, weights)); 
	}

	aiReleaseImport (scene);
	printf ("\nMesh loaded.\n");

	delete[] buffers;
	
	return true;
}

void Model::Render(GLuint shader)
{
	glBindVertexArray(vao);
		
	if(wireframe)
		glPolygonMode(GL_FRONT, GL_LINE); 

	if(meshEntries.size() > 1)
	{
		for(int meshEntryIdx = 0; meshEntryIdx < meshEntries.size(); meshEntryIdx++)
		{
			//const GLuint materialIndex = meshEntries[meshEntryIdx].MaterialIndex; //Each mesh entry has one material
			//assert(materialIndex < textures.size());

			glActiveTexture(GL_TEXTURE0);
			//glUniform1i(glGetUniformLocation(shader, "texture_diffuse"), 0); //set the sampler in the shader to the correct texture 
			glBindTexture(GL_TEXTURE_2D, meshEntries[meshEntryIdx].TextureIndex);

			glDrawElementsBaseVertex(GL_TRIANGLES, 
                                meshEntries[meshEntryIdx].NumIndices, 
                                GL_UNSIGNED_INT, 
                                (void*)(sizeof(unsigned int) * meshEntries[meshEntryIdx].BaseIndex), 
                                meshEntries[meshEntryIdx].BaseVertex);
		}
	}
	else if(indices.size() > 0)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, meshEntries[0].TextureIndex);

		glDrawElements( GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0);
	}
	else
	{
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);	
	}

	if(wireframe)
		glPolygonMode(GL_FRONT, GL_FILL); 

	// Make sure the VAO is not changed from the outside    
    //glBindVertexArray(0); //?
}

GLuint Model::LoadTexture(const char* fileName) 
{		
	Magick::Blob blob;
	Magick::Image* image; 

	string stringFileName(fileName);
	string fullPath = "Textures/" + stringFileName;

	try {
		image = new Magick::Image(fullPath.c_str());
		image->write(&blob, "RGBA");
	}
	catch (Magick::Error& Error) {
		std::cout << "Error loading texture '" << fullPath << "': " << Error.what() << std::endl;

		delete image;
		return false;
	}

	GLuint textureID;

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
			
	//Load the image data in to the texture
	glTexImage2D(GL_TEXTURE_2D, 0/*LOD*/, GL_RGBA, image->columns(), image->rows(), 0/*BORDER*/, GL_RGBA, GL_UNSIGNED_BYTE, blob.data());

	//Parameter stuff, for magnifying texture etc.
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
			
	glBindTexture(GL_TEXTURE_2D, 0); //unbind try now without animation and see if there is that initial error

	delete image;  
	return textureID;
}