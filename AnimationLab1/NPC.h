#pragma once

#include "Model.h"
#include "Common.h"
#include "Player.h"

namespace NPCns {
enum State { idle = 0, wave, talk };
}

class NPC //TODO inherist from Character
{
	private:

		int state;
		Skeleton* skeleton;

	public:

		Model *model;
		Player *player;

		float threshold;
		bool playerInRadius;

		bool haveAcknowledged;

		NPC(vector<Model*> &objectList, Model* model, Player* player);
		~NPC(){};

		void Update(double deltaTime);
		void ProcessKeyboardOnce(unsigned char key, int x, int y);
		
		void SetState(NPCns::State newState);

		void LoadAnimation(const char* fileName) { model->GetSkeleton()->LoadAnimation(fileName); }

};