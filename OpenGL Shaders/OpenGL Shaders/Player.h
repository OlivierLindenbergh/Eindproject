#ifndef _PLAYER_
#define _PLAYER_

#include "Header.h"
#include "Model.h"
#include "AnimationTypes.h"

extern std::vector<gameobject> animationModels;
extern std::vector<s_anim> playerAnimations;

extern double deltaTime;

extern glm::vec2 handleMouseInput(bool trapMouseInWindow);
extern float mouseSensitivity;
#include "Gun.h"

class Player : public Model
{
public:
	Player();
	~Player();

	void init(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, float _maxHealth);
	void update();

	void setAttackStrength(float dmg);

	gameobject *getAnimModel();

	static s_anim loadAnimations(char * animationFolder, std::vector<gameobject> &gameobjects, int startframe, int frames, double fps, bool loop);
	static void createAnimationModels();
	static GLuint animationTexture;

	void updateAnimation(int currentType);
	void resetAnimation();

	bool canUse(glm::vec3 useObjectOrigin, float maxDist);

	float health, maxHealth;
	glm::vec3 speed;
	float attackStrength;

	int ammo, points;

	Gun gun;

	bool shooting, wantsToShoot, canShoot;
	double shootTimer;

	int networkAnimType;

	bool active;

private:

	int currentAnimationType,
		currentAnimationFrame;
	double animationExtraTime;
};

#endif