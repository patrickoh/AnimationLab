#ifndef _OBJ3D_H                // Prevent multiple definitions if this 
#define _OBJ3D_H                // file is included in more than one place

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 

#include <assimp/cimport.h> // C importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <assert.h>

#include <string> 
#include <fstream>
#include <iostream>
#include <sstream>

#include <map>
#include <vector>

#include "Bone.h"
#include "helper.h"
#include "Skeleton.h"

using namespace std;

enum VB_TYPES //TODO - support models with different combinations of below
{
	POS_VB,
	NORMAL_VB,
	TEXCOORD_VB,
	BONE_VB,
	WEIGHT_VB,
	INDEX_VB,
	NUM_VBs            
};

struct WorldProperties
{
	glm::vec3 translation;
	glm::mat4 orientation;
	glm::vec3 scale;

	WorldProperties()
	{
		translation = glm::vec3(0);
		orientation = glm::mat4(1);
		scale = glm::vec3(0);
	}
};

struct MeshEntry {
    
	MeshEntry()
    {
        NumIndices    = 0;
        BaseVertex    = 0;
        BaseIndex     = 0;
    }
        
    unsigned int NumIndices;
    unsigned int BaseVertex;
    unsigned int BaseIndex;
};

class Model
{
	private:

		GLuint vao;
		vector<int> indices;

		GLuint shaderProgramID;

		int vertexCount;

		Skeleton* skeleton;
		bool hasSkeleton;
		
	public:
		
		Model(glm::vec3 position, glm::mat4 orientation, glm::vec3 scale, const char* file_name, GLuint shaderProgramID);
		~Model();

		WorldProperties worldProperties;
		vector<MeshEntry> MeshEntries;

		bool Load(const char* file_name);

		//Getters
		GLuint GetVAO() { return vao; }
		GLuint GetShaderProgramID() { return shaderProgramID; }
		int GetVertexCount() { return vertexCount; }
		Skeleton* GetSkeleton() { return skeleton; }
		bool HasSkeleton() { return hasSkeleton; }
		vector<int> GetIndices() { return indices; }
		
		glm::mat4 GetModelMatrix() 
		{ 
			return glm::translate(glm::mat4(1.0f), worldProperties.translation) 
				* worldProperties.orientation
				* glm::scale(glm::mat4(1.0f), worldProperties.scale);
		}		

		//glm::vec3 GetWorldPosition() { return worldProperties.translation; }

		//Setters
		void SetShaderProgramID(GLuint p_shaderProgramID) { shaderProgramID = p_shaderProgramID; }
};

#endif