#ifndef _SKELETON_H                // Prevent multiple definitions if this 
#define _SKELETON_H                // file is included in more than one place

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <assimp/cimport.h> // C importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <assert.h>

#include <vector>
#include <map>

#include "helper.h"
#include "Bone.h"
//#include "Model.h"

#include <glm\gtx\quaternion.hpp>

class Model;

class Skeleton
{
	private:
		
		Model* model;

		std::map<int, Bone*> bones;
		std::map<std::string, int> boneNameToID;

		//animationIndex 0- walk, 1-run etc.

		double animationDuration;
		double animationTimer;

		float animationSpeedScalar;

		std::vector<std::string> bonesAdded;

		Bone* GetBone(Bone* root, const char* name);

	public:
		Bone* root;

		bool hasKeyframes;
		std::map<std::string, std::vector<Bone*>> ikChains;
		GLuint line_vao;

		static bool ConstraintsEnabled;

		Skeleton(Model* myModel, bool keyframes);
		virtual ~Skeleton();

		bool ImportAssimpBoneHierarchy(const aiScene* scene, aiNode* aiBone, Bone* parent);
		void Animate(double deltaTime);

		//void Control(bool *keyStates);
		
		bool ComputeIK(std::string chainName, glm::vec3 D, int steps);
		void DefineIKChain(std::string name, std::vector<Bone*> chain);
		void ImposeDOFRestrictions(Bone* bone);

		void UpdateGlobalTransforms(Bone* bone, glm::mat4 parentTransform);

		void PrintHeirarchy(Bone* root);

		//Getters
		std::map<int, Bone*> GetBones() { return bones; }
		
		Bone* GetBone(int id) { return bones[id]; }
		Bone* GetBone(std::string name);

		Bone* operator [](int i) { return bones[i]; }
		Bone* GetRootBone() { return root; }
		
		double GetAnimationTimer() { return animationTimer; }

		//Setters
		void SetAnimDuration(double pAnimationDuration) { animationDuration = pAnimationDuration; }
};

#endif