#include "ClientGame.h"

#include "WEngine/Usage.h"

#include "Enemy.h"
#include "Model.h"
#include "Player.h"

#define WAVE_SPEED 0.03

#define MSAA 1
#define SSAA 2
#define FXAA 3

#define UPDATE_CYCLES_PER_SECOND 144
clock_t programStartClock = std::clock();

// ============  Client Handle Variables ===========
bool clientRunning = false;
ClientGame client;
void clientLoop(void *);

double	clientLoopDeltaTime		 = 0,
		clientLoopLastRenderTime = 0;

// ============  GAME PROGRAM VARIABLES ============

GLFWwindow* window;
void (*networkUpdateFunction)(void) = nullptr;

float mouseSensitivity = 1.0f;

Player player;
Player otherPlayers[MAX_LOBBYSIZE];

Loader loader;
Camera camera;

MasterRenderer modelRenderer,
			   terrainRenderer,
			   normalModelRenderer;
WaterMasterRenderer waterRenderer;
ShadowMasterRenderer shadowRenderer;
GuiRenderer guiRenderer;

//Only render scene to sceneRenderer framebuffer. Then retrieve this texture for use.
PostProcessRenderer sceneRenderer, antiAliasedRenderer;
//Post Process image 
PostProcessRenderer Contrast, VBlur, HBlur, brightFilter, combineFilter;


// == RENDER ELEMENTS ==

GLuint SA_T_Building[5];
GLuint SA_T_AmmoBoxes[2], SA_TN_AmmoBoxes[2];
GLuint SA_T_Barrels;
GLuint SA_T_Barriers, SA_TN_Barriers;
GLuint SA_T_Crate, SA_TN_Crate;
GLuint SA_T_Crate2;
GLuint SA_T_Pallets, SA_TN_Pallets;
GLuint SA_T_SandBag, SA_TN_SandBag;
GLuint SA_T_SandBag2;

Model SA_M_Building[1 * 5];
Model SA_M_AmmoBoxes[3 * 7];
Model SA_M_Barrels[5];
Model SA_M_Barriers[4];
Model SA_M_Crate2;
Model SA_M_Crate[6];
Model SA_M_Pallets[2];
Model SA_M_SandBag,
	  SA_M_SandBag2;

std::vector<texture2D> GuiElements;
texture2D GuiCherry;

Terrain terrains[2];

std::vector<Light*> lights;
Light sun, light0, light1, light2;

WaterTile water;

Skybox skybox;

textureCubemap waterReflection;

// ===  GAME INFO  ===
glm::vec3 clearColor = glm::vec3(0, 0, 0);

double last_render_time = 0;
double deltaTime = 0,
	   frameStartTime = 0,
	   debugTime = 0;
float fps = 0;
int frame = 0;
int gameState = 0;

int Max_Fps = 60;
bool limit_fps = true;

/* The Anti-Aliasing type used in the program.
   Can be one of the following:
   * MSAA - MultiSampled Anti-Aliasing
   * SSAA - SuperSampled Anti-Aliasing :! Under Contstruction !:
   * FXAA - Fast Approximate Anti-Aliasing
*/
int AAType = FXAA;

// ===  FUNCTIONS  ===

// Load a model with only a texture
void loadModel(Model &model, std::string modelFilename, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, GLuint textureID, float shineDamper, float reflectivity, float ambientLight);
// Load a model with a texture and normalMap
void loadModel(Model &model, std::string modelFilename, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, GLuint textureID, GLuint normalTextureID, float shineDamper, float reflectivity, float ambientLight);
// Load a model with a texture, normalMap and a shadowMap
void loadModel(Model &model, std::string modelFilename, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, GLuint textureID, GLuint normalTextureID, GLuint shadowMapID, float shineDamper, float reflectivity, float ambientLight);

// Set a model with only a texture
void loadModel(Model &model, Model &oriModel, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, GLuint textureID, float shineDamper, float reflectivity, float ambientLight);
// Set a model with a texture and normalMap
void loadModel(Model &model, Model &oriModel, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, GLuint textureID, GLuint normalTextureID, float shineDamper, float reflectivity, float ambientLight);
// Set a model with a texture, normalMap and a shadowMap
void loadModel(Model &model, Model &oriModel, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, GLuint textureID, GLuint normalTextureID, GLuint shadowMapID, float shineDamper, float reflectivity, float ambientLight);

// If trapMouseInWindow it returns the changed amount of pixels since last update
// Else if returns the position the mouse is on
glm::vec2 handleMouseInput(bool trapMouseInWindow);
// Handle user input
void handleGameInput();
// Update time stuff
void updateTime();
// Render water textures
void renderWaterTextures();
// Render shadow textures
void renderShadowTexture(Light *shadowLight);
// Render water cubemap : WORK IN PROGRESS :
void renderWaterCubeMap();

// Load and initialise all waterTiles
void loadAndInitialiseWater();
// Load and initialise all GUI elements
void loadAndInitialiseGUI();
// Load and initialise all framebuffers
void loadAllFrameBuffers();
// Load and initialise all renderers
void loadAndInitialiseRenderers();
// Load and initialise the skybox
void loadSkybox();
// Initialise Lights
void initLights();

// Initialise the client
void initialiseClient(char ipAddress[39], char port[5]);
// Client loop
void clientLoop(void *);

// Network functions
void SendInitData();
void SendLobbyData();
void SendGameData();

// Load all graphics
void loadGraphics();
// Load Safe Area Graphics
void LoadGraphics_SafeArea();
// Load all models
void loadModels();
// Load Safe Area Models
void loadModels_SafeArea();

int main() {

	// ============  NETWORKING LOGIC =================

	// Initialise, set the client and connect to the server.
	initialiseClient("127.0.0.1", "6881");


	// ===============  GAME LOGIC ====================

	// Initialise GLFW and throw error if it failed
	if (!glfwInit()){
		throw std::runtime_error("Failed to initialize GLFW");
		exit(-1);
	}

	// ========  Initialise  ==========
	last_render_time = glfwGetTime();


//TODO: StartScreen Loop here

	// ===  DISPLAY  ===
	//Create a display and initialise GLEW
	DisplayManager::createDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, PROGRAM_NAME, false);

	//Just disable vsync for max fps :).
	DisplayManager::disableVsync();

	// ===  CAMERA  ===
	//Intialise the camera and set its position and rotation
	camera.Initalise(75.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.01f, 1000.0f);
	camera.Set(glm::vec3(0, 10, -50), glm::vec3(0, _PI, 0));

	// ===  Framebuffers  ===

	//Initialise framebuffer for cubemap texture waterReflection
	waterReflection.initialseFrameBuffer(1280);

	// Load and initialise all framebuffers
	loadAllFrameBuffers();
	// Load and initialise all renderers
	loadAndInitialiseRenderers();

	// ===  SKYBOX  ===
	loadSkybox();

	// ===  GUI  ===
	loadAndInitialiseGUI();
	
	// ===  INPUT  ===
	//Set input mode
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	//Set input callback
	//glfwSetKeyCallback(window, keyfun);

	// ===  LOAD AND INITIALISE MODELS  ===
	// Load all graphics
	loadGraphics();
	// Load all models and initialise
	loadModels();

	// ===  LIGHTS  ===
	initLights();
	
	// ===  WATER  ===
	loadAndInitialiseWater();

	//renderWaterCubeMap();

	// ===  Add GUI elements to the renderig list  ===
	//GuiElements.push_back(GuiCherry);

	//Initialise gameState
	gameState = 1;

	// Hide the cursor
	DisplayManager::gameCursor();
//TODO: glfwCreateCursor can set a picture of the cursor

	// Set the mouse in the middle of the screen at start so there is no unwanted staring rotation
	handleMouseInput(true);
	
	do {
		//renderWaterCubeMap();
		//GuiElements[0].rotation = (float)frame / 100.0f;

		//Prepare rendering on default framebuffer
		MasterRenderer::prepare();

		//Change WaterMoveFactor for random(ish) water movement
		float currentWaterMoveFactor = waterRenderer.getMoveFactor();
		currentWaterMoveFactor += (float)(WAVE_SPEED * deltaTime);
		if (currentWaterMoveFactor > 1) currentWaterMoveFactor -= 1;
		waterRenderer.setMoveFactor(currentWaterMoveFactor);
		
		/* ========= Add models to list for rendering ============== */
		
		//Add terrains to renderer list.
		terrainRenderer.addToRenderList(terrains[0].getModel());
		terrainRenderer.addToRenderList(terrains[1].getModel());

		//Add normalmapped models to renderer list
		normalModelRenderer.addToRenderList(model.getModel());
		normalModelRenderer.addToRenderList(model2.getModel());

		//Add lantern models
		for (int i = 0; i < 4; i++) modelRenderer.addToRenderList(lamps[i].getModel());

		//Add water to the renderer list
		waterRenderer.addToRenderList(&water);
		
		/* =============== Start of rendering ===================== */
		
		//Render reflection and refraction texture of the water
		renderWaterTextures();
		
		//Render the shadow texture
		renderShadowTexture(&sun);

		glFinish();
		frameStartTime = glfwGetTime();

		//Render scene to multisampled framebuffer
		sceneRenderer.bindRenderToTextureFrameBuffer();
			MasterRenderer::prepare();

			//Render everything
			skybox.render(&camera);

			terrainRenderer.render(lights, &camera, glm::vec4(0, -1, 0, 100000));
			waterRenderer.render(lights, &camera);

			modelRenderer.render(lights, &camera, glm::vec4(0, -1, 0, 100000));
			normalModelRenderer.render(lights, &camera, glm::vec4(0, -1, 0, 100000));
		
		sceneRenderer.unbindFrameBuffer();

		glFinish();
		debugTime = glfwGetTime();

		/* =================== Post processing below! ============== */

		// Handle antialiasing
		if (AAType == MSAA) {
			//Resolve multisampled framebuffer to antialiased framebuffer
			sceneRenderer.resolveTo(&antiAliasedRenderer);
		}
		else if (AAType == SSAA) {
			//TODO: Create a way to resolve a SuperSampled framebuffer
		}
		else if (AAType == FXAA) {
			//Render image to antiAliased buffed with the FXAA Shader
			antiAliasedRenderer.renderToFrameBuffer(sceneRenderer.getOutputTexture());
		}

		//antiAliasedRenderer.renderToScreen();
		
		//Renderer antialiased framebuffer to brightfilter
		brightFilter.renderToFrameBuffer(antiAliasedRenderer.getOutputTexture());
		brightFilter.renderToScreen();

		//Blur the brightfilter
		HBlur.renderToFrameBuffer(brightFilter.getOutputTexture());
		VBlur.renderToFrameBuffer(HBlur.getOutputTexture());

		//Combine the brightfiltered framebuffer and the antialiased framebuffer into one
		combineFilter.shader.loadHighlightAmount(1.0f);
		combineFilter.renderToFrameBuffer(antiAliasedRenderer.getOutputTexture(), VBlur.getOutputTexture());
		//combineFilter.renderToScreen();

		//Rendered combined image to contrast framebuffer
		Contrast.shader.loadContrast(0.02f);
		Contrast.renderToFrameBuffer(combineFilter.getOutputTexture());

		//Render contrast framebuffer to the screen
		Contrast.renderToScreen();

		// =====  GUI ELEMENTS =====

		// Render GuiElements
		guiRenderer.render(&GuiElements);
		
		//Swap Buffers (Send image to the screen)
		glfwSwapBuffers(window);
	
		/* ================== End of rendering ======================= */

		//Clear all objects from renderlists.
		modelRenderer.clearRenderPass();
		normalModelRenderer.clearRenderPass();
		terrainRenderer.clearRenderPass();
		waterRenderer.clearRenderPass();


		//Update all time related variables.
		updateTime();

		if (limit_fps) {
			// Limit fps to FPSLIMIT
			while (glfwGetTime() < (last_render_time + (1.0f / (float)Max_Fps))) {}
		}

		// Poll all events
		glfwPollEvents();

		// Update mouse input and lock it in the middle of the screen
		glm::vec2 changedMousePos = handleMouseInput(true);
		changedMousePos *= mouseSensitivity;

		// Rotate the x and y axis of the player with the mouse
		glm::vec3 rot = player.getRotation();
		rot.y -= changedMousePos.x * 0.1f;
		rot.x -= changedMousePos.y * 0.1f;

		if (rot.x < -90.0f) rot.x = -90.0f;
		else if (rot.x > 90.0f) rot.x = 90.0f;
		
		player.setRotation(rot);

		// Get and update the player with its new position, it's health and the rest
		client.getPlayerData(player);

		// Set the rotation of the player for the next update
		client.setPlayerData(player);
		
		// Set the camera rotation to the players rotation and convert it to radians
		camera.rotation = glm::radians(player.getRotation());
		// Set the camera position
		//camera.position = player.getPosition();
		//camera.position.y += 10.0f;

		//Set title to hold fps info
		std::string fpsStr = std::string(PROGRAM_NAME) + " FPS: " + std::to_string(fps) + " deltaTime: " + std::to_string(deltaTime * 100) + " Mouse: x: " + std::to_string(rot.x) + " y: " + std::to_string(rot.y);
		DisplayManager::setDisplayTitle(fpsStr.c_str());

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) gameState = 0;
		if (DisplayManager::updateDisplay() < 0) gameState = 0;
	}while (gameState >= 1);

	// Show the cursor again
	DisplayManager::showCursor();

	//Close and Cleanup the display
	DisplayManager::closeDisplay();

	//Clean all programs up
	modelRenderer.cleanUp();
	normalModelRenderer.cleanUp();
	terrainRenderer.cleanUp(); 
	waterRenderer.cleanUp();
	shadowRenderer.cleanUp();
	
	loader.cleanUp();

	//Make the clientloop stop running and end its thread
	clientRunning = false;

	//Terminate glfw
	glfwTerminate();
}



// If trapMouseInWindow it returns the changed amount of pixels since last update
// Else if returns the position the mouse is on
glm::vec2 handleMouseInput(bool trapMouseInWindow) {
	// Get mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	glm::vec2 windowPos = DisplayManager::getWindowPosition();
	glm::vec2 windowSize = DisplayManager::getWindowSize();

	glm::vec2 windowCenter = glm::vec2(windowPos.x + (windowSize.x / 2), windowPos.y + (windowSize.y / 2));

	glm::vec2 difference;

	// Reset mouse position for next frame	
	if (trapMouseInWindow) {
		difference = glm::vec2(windowCenter.x - xpos, windowCenter.y - ypos);
		glfwSetCursorPos(window, windowCenter.x, windowCenter.y);
	}
	else {
		difference = glm::vec2(xpos, ypos);
	}

	return difference;
}
// Handle user input in game
void handleGameInput()
{
	// Poll all events
	glfwPollEvents();

	// HandleInput from keys

	// Handle input of forward or backward
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) client.addActionType(MOVE_FORWARD);
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) client.addActionType(MOVE_BACKWARD);
	// Handle input of left or right
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) client.addActionType(MOVE_LEFT);
	else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) client.addActionType(MOVE_RIGHT);

	// Handle input of sprinting
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) client.addActionType(MOVE_RUN);
	
	// Handle input of shooting
	if (glfwGetKey(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) client.addActionType(SHOOT_ONCE);
	
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) client.addActionType(JUMP);
}
// Update time stuff
void updateTime()
{
	// Calculate delta time
	//glfwGetTime() nr of seconds since start of game
	//last_render_time last set time nr seconds since start of game
	deltaTime = glfwGetTime() - last_render_time; //Time in seconds it took for the last frame.
	fps = (float)(1.0f / deltaTime);

	last_render_time = glfwGetTime();

	frame++;
}

// Load and initialise all waterTiles
void loadAndInitialiseWater()
{
	//Create a waterTile and set its attributes
	water = WaterTile(glm::vec3(0, 2, -400), glm::vec3(0, 0, 0), glm::vec3(800, 500, 400));
	water.setReflectionTexture(waterRenderer.getReflectionTexture());
	water.setRefractionTexture(waterRenderer.getRefractionTexture());
	water.setDuDvTexture(loader.loadTexture("res/Water/WaterDuDvMap.bmp", false));
	water.setNormalMapTexture(loader.loadTexture("res/Water/WaterNormalMap.bmp", false));
	water.setRefractionDepthTexture(waterRenderer.getRefractionDepthTexture());
	water.getWaterTile()->setShineDamper(100);
	water.getWaterTile()->setReflectivity(1);
	water.getWaterTile()->setAmbientLight(0.5f);
	water.getWaterTile()->setShadowMap(shadowRenderer.getShadowDepthTexture());
}
// Load and initialise all GUI elements
void loadAndInitialiseGUI()
{
	// Load cherry tree as GUI element
	GuiCherry.loadImage("res/Cherry/cherry.bmp", true, false);
	GuiCherry.rotation = glm::radians(10.0f);
	GuiCherry.scale = glm::vec2(0.5f);
	GuiCherry.position = glm::vec2(-0.5f, 0.5f);
}
// Render water textures
void renderWaterTextures()
{
	//Enable clipdistance 0
	glEnable(GL_CLIP_DISTANCE0);

	//Bind Reflection framebuffer
	waterRenderer.bindReflectionFrameBuffer();
	//Prepare rendering on the reflection framebuffer
	MasterRenderer::prepare();

	//Position the camera down to get the proper reflection
	float distance = 2 * (camera.position.y - water.getWaterTile()->getPosition().y);
	camera.position.y -= distance;
	camera.rotation.x *= -1; //Invert pitch

							 //Render all objects to the reflection of the water
	skybox.render(&camera);
	terrainRenderer.render(lights, &camera, glm::vec4(0, 1, 0, -water.getWaterTile()->getPosition().y + 1.0f));
	//modelRenderer.render(lights, &camera, glm::vec4(0, 1, 0, -water.getWaterTile()->getPosition().y + 1.0f));
	//normalModelRenderer.render(lights, &camera, glm::vec4(0, 1, 0, -water.getWaterTile()->getPosition().y));

	camera.position.y += distance; //Position the camera back up
	camera.rotation.x *= -1; //Invert pitch back to normal

							 //Bind Refraction framebuffer
	waterRenderer.bindRefractionFrameBuffer();
	//Prepare rendering on the Refraction framebuffer
	MasterRenderer::prepare();

	//Render all elements to the refraction of the water
	terrainRenderer.render(lights, &camera, glm::vec4(0, -1, 0, water.getWaterTile()->getPosition().y + 1.0f));
	//modelRenderer.render(lights, &camera, glm::vec4(0, -1, 0, water.getWaterTile()->getPosition().y + 1.0f));
	//normalModelRenderer.render(lights, &camera, glm::vec4(0, -1, 0, water.getWaterTile()->getPosition().y));

	waterRenderer.unbindCurrentFrameBuffer();

	//Disable clipdistance 0
	glDisable(GL_CLIP_DISTANCE0);
}
// Render shadow textures
void renderShadowTexture(Light *shadowLight)
{
	//Render all objects that have to cast a shadow to the shadowmap
	shadowRenderer.bindShadowFrameBuffer();

	//Render all objects to the shadow buffer
	//shadowRenderer.render(shadowLight, &camera, &terrainRenderer.gameobjects);
	shadowRenderer.render(shadowLight, &camera, &modelRenderer.gameobjects);
	shadowRenderer.render(shadowLight, &camera, &normalModelRenderer.gameobjects);

	shadowRenderer.unbindCurrentFrameBuffer();
}

// Render water cubemap : WORK IN PROGRESS :
void renderWaterCubeMap()
{
	//Add normalmapped models to renderer list
	normalModelRenderer.addToRenderList(model.getModel());
	normalModelRenderer.addToRenderList(model2.getModel());
	//Add terrain models to renderer list
	terrainRenderer.addToRenderList(terrains[0].getModel());
	terrainRenderer.addToRenderList(terrains[1].getModel());

	glm::vec3 reflectionPosition = glm::vec3(0.0f, 10.0f, 0.0f); //Has to be a vec3 of the position
	const glm::vec3 faceRotations[6] = {
		glm::vec3(0, glm::radians(-90.0f), glm::radians(180.0f)),
		glm::vec3(0, glm::radians(90.0f),  glm::radians(180.0f)),
		glm::vec3(glm::radians(-90.0f), 0, 0),//
		glm::vec3(glm::radians(90.0f),  0, 0),//
		glm::vec3(0, glm::radians(180.0f), glm::radians(180.0f)),
		glm::vec3(0, 0,	glm::radians(180.0f))
	};

	waterReflection.bindFrameBuffer();
	for (unsigned int i = 0; i < 6; i++) {
		//Bind the right side
		waterReflection.bindFrameBufferRenderTexture(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
		//Clear the buffer
		MasterRenderer::prepare();
		//Render

		skybox.renderUpdated(&camera, 90.0f, reflectionPosition, faceRotations[i]);

		//normalModelRenderer.renderUpdated(lights, &camera, glm::vec4(0, -1, 0, 100000), 90.0f, reflectionPosition, faceRotations[i]); //Normal render
		terrainRenderer.renderUpdated(lights, &camera, glm::vec4(0, -1, 0, 100000), 90.0f, reflectionPosition, faceRotations[i]); //Normal render

		//glEnable(GL_CLIP_DISTANCE0);
		//normalModelRenderer.renderUpdated(lights, &camera, glm::vec4(0, 1, 0, -water.getWaterTile()->getPosition().y + 1.0f), 90.0f, reflectionPosition, faceRotations[i]); //Reflection render
		//normalModelRenderer.renderUpdated(lights, &camera, glm::vec4(0, -1, 0, water.getWaterTile()->getPosition().y + 1.0f), 90.0f, reflectionPosition, faceRotations[i]); //Refraction render
		//glDisable(GL_CLIP_DISTANCE0);
	}
	waterReflection.unbindFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT);
	//waterReflection.deleteBuffers(); //Only if we dont want to use the framebuffer ever again

	normalModelRenderer.clearRenderPass();
	terrainRenderer.clearRenderPass();
}

// Load and initialise all framebuffers
void loadAllFrameBuffers()
{
	//Load and intialise all postprocessors
	if (AAType == MSAA) {
		sceneRenderer.load(glm::vec2(SCREEN_WIDTH, SCREEN_HEIGHT), 8);
		antiAliasedRenderer.load(glm::vec2(SCREEN_WIDTH, SCREEN_HEIGHT));
	}
	else if (AAType == SSAA) {
		sceneRenderer.load(glm::vec2(SCREEN_WIDTH * 4, SCREEN_HEIGHT * 4));
		//TODO: create a shader that downsamples the image created!
		antiAliasedRenderer.load(glm::vec2(SCREEN_WIDTH, SCREEN_HEIGHT));
	}
	else if (AAType == FXAA) {
		sceneRenderer.load(glm::vec2(SCREEN_WIDTH, SCREEN_HEIGHT));
		antiAliasedRenderer.load("WEngine/Shaders/AntiAliasing/AAVertex.vs", "WEngine/Shaders/AntiAliasing/FXAA.fs", glm::vec2(SCREEN_WIDTH, SCREEN_HEIGHT));
		antiAliasedRenderer.shader.loadInverseFilterTextureSize(glm::vec3((1.0f / SCREEN_WIDTH), (1.0f / SCREEN_HEIGHT), 0.0f));
		antiAliasedRenderer.shader.loadFXAAParameters((8.0f), (1.0f / 128.0f), (1.0f / 8.0f));
	}
	Contrast.load("WEngine/Shaders/PostProcessing/ContrastEffect.vs", "WEngine/Shaders/PostProcessing/ContrastEffect.fs", glm::vec2(SCREEN_WIDTH, SCREEN_HEIGHT));
	VBlur.load("WEngine/Shaders/PostProcessing/VerticalGaussionBlur.vs", "WEngine/Shaders/PostProcessing/GaussionBlur.fs", glm::vec2(SCREEN_WIDTH / 5, SCREEN_HEIGHT / 5));
	HBlur.load("WEngine/Shaders/PostProcessing/HorizontalGaussionBlur.vs", "WEngine/Shaders/PostProcessing/GaussionBlur.fs", glm::vec2(SCREEN_WIDTH / 5, SCREEN_HEIGHT / 5));
	brightFilter.load("WEngine/Shaders/PostProcessing/simpleVertex.vs", "WEngine/Shaders/PostProcessing/brightFilter.fs", glm::vec2(SCREEN_WIDTH, SCREEN_HEIGHT));
	combineFilter.load("WEngine/Shaders/PostProcessing/simpleVertex.vs", "WEngine/Shaders/PostProcessing/combineFilter.fs", glm::vec2(SCREEN_WIDTH, SCREEN_HEIGHT));
	combineFilter.shader.start();
	combineFilter.shader.connectTextureUnits();
	combineFilter.shader.stop();

}
// Load and initialise all masterRenderers
void loadAndInitialiseRenderers()
{
	//Load and initialise all masterRenderers
	guiRenderer.load("WEngine/Shaders/Gui/GuiShader.vs", "WEngine/Shaders/Gui/GuiShader.fs");
	modelRenderer.load("WEngine/Shaders/Default/Default.vs", "WEngine/Shaders/Default/Default.fs", &camera);
	normalModelRenderer.load("WEngine/Shaders/NormalMaps/NormalMap.vs", "WEngine/Shaders/NormalMaps/NormalMap.fs", &camera);
	terrainRenderer.load("WEngine/Shaders/Terrain/Terrain.vs", "WEngine/Shaders/Terrain/Terrain.fs", &camera);
	waterRenderer.load("WEngine/Shaders/Water/WaterShader.vs", "WEngine/Shaders/Water/WaterShader.fs", &camera);
	shadowRenderer.load("WEngine/Shaders/Shadows/ShadowShader.vs", "WEngine/Shaders/Shadows/ShadowShader.fs");
}
// Load and initialise the skybox
void loadSkybox()
{
	std::string skyboxTextures[6] = {
		"res/Skybox/hw_desertnight/desert_night_right.bmp", // RIGHT
		"res/Skybox/hw_desertnight/desert_night_left.bmp",  // LEFT
		"res/Skybox/hw_desertnight/desert_night_top.bmp",	// TOP
		"res/Skybox/hw_desertnight/desert_night_bottom.bmp",// BOTTOM
		"res/Skybox/hw_desertnight/desert_night_back.bmp",	// BACK
		"res/Skybox/hw_desertnight/desert_night_front.bmp",	// FRONT
	};
	skybox.load("WEngine/Shaders/Skybox/Skybox.vs", "WEngine/Shaders/Skybox/Skybox.fs", &camera, skyboxTextures);
}
// Initialise Lights
void initLights()
{
	//Initialise lights and add them to the light list
	sun = Light(glm::vec3(0, 1000, -7000), glm::vec3(0.35f, 0.35f, 0.35f));
	lights.push_back(&sun);

	light0 = Light(glm::vec3(185, terrains[1].getHeight(185, -293) + 13.2f, -293), glm::vec3(2, 0, 0), glm::vec3(1, 0.01f, 0.002f)); //Red
	lights.push_back(&light0);

	light1 = Light(glm::vec3(370, terrains[1].getHeight(370, -300) + 13.2f, -300), glm::vec3(0, 2, 2), glm::vec3(1, 0.01f, 0.002f));
	lights.push_back(&light1);

	light2 = Light(glm::vec3(293, terrains[1].getHeight(293, -305) + 13.2f, -305), glm::vec3(2, 2, 0), glm::vec3(1, 0.01f, 0.002f));
	lights.push_back(&light2);
}

// ======  SERVER HANDLE FUNCTIONS  ======

// Intialise the client and connect to the server on ipAddress and port
void initialiseClient(char ipAddress[39], char port[5])
{
	// Initialise the client, create a connection and try to connect to the ip and port of the server.
	client = ClientGame(ipAddress, port);

	// Check if there are any intialisation errors. 
	// If this error is fatal, we stop creating the clientNetwork.
	std::vector<networkingErrors> clientErrors = client.getErrors();
	for (unsigned int i = 0; i < clientErrors.size(); i++)
	{
		switch (clientErrors[i]) {
		case WSA_STARTUP_ERROR:
			printf("ERROR -- Creating client WSAStartup\n");
			return;
			break;
		case GET_ADDR_INFO_ERROR:
			printf("ERROR -- Creating client getaddrinfo\n");
			return;
			break;
		case CREATE_SOCKET_ERROR:
			printf("ERROR -- Creating client createSocket\n");
			return;
			break;
		case CONNECT_SOCKET_ERROR:
			printf("ERROR -- Creating client connectSocket\n");
			break;
		case ALL_CONNECTING_SOCKETS_ERROR:
			printf("ERROR -- Creating client connecting all sockets\n");
			return;
			break;
		case SET_NONBLOCKING_ERROR:
			printf("ERROR -- Creating client nonblocking\n");
			return;
			break;
		}
	}

	networkUpdateFunction = SendInitData;

	// Start a new thread and run the serverLoop function.
	_beginthread(clientLoop, 0, NULL);
}
// The client loop
void clientLoop(void *)
{
	clientRunning = true;

	// Client networking loop
	while (clientRunning) {

		// Receive and parse data.
		client.updateClient();

		// If the function pointer is set to a function, execute this function.
		// This function is to execute a function that sends data to the server.
		if (networkUpdateFunction != nullptr) networkUpdateFunction();

		// Limit update cycle amount to UPDATE_CYCLES_PER_SECOND
		while (((float)(std::clock() - programStartClock) / (float)CLOCKS_PER_SEC) < (clientLoopLastRenderTime + (1.0f / (float)UPDATE_CYCLES_PER_SECOND))) {}

		clientLoopDeltaTime = (((float)(std::clock() - programStartClock) / (float)CLOCKS_PER_SEC)) - clientLoopLastRenderTime; //Time in miliseconds it took for the last update cycle.
		clientLoopLastRenderTime = (((float)(std::clock() - programStartClock) / (float)CLOCKS_PER_SEC));
	}

	// End of function, end the thread and release its memory.
	_endthread();
}

// Send intitialisation data
// When just connected to the server and need to send name.
void SendInitData()
{
	char name[] = "Wouter140";

	memcpy(&client.myPlayerData.playerName, name, strlen(name));
	client.myPlayerData.playerNameSize = strlen(name);
	client.addActionType(GAME_INITIALISATION);

	client.sendLobbyUpdate();
}
// Send lobby data
// When in lobby
void SendLobbyData()
{
	if (/*want to start the game*/)
	{
		// Add a action Start Game
		client.addActionType(GAME_START);
		// Send the packet to the server
		client.sendLobbyUpdate();
	}

	// If the lobbyTimer has started running
	if (client.gameStarting)
	{

		client.lobbyTimer -= (float)clientLoopDeltaTime;
		if (client.lobbyTimer <= 0.0f) client.lobbyTimer = 0.0f;
	}
}

// Send game data
// When in game
void SendGameData()
{
	// Update key input
	handleGameInput();
}

// Load all graphics
void loadGraphics()
{
	floorTextureGrass = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	floorTextureR = loader.loadTexture("res/Terrain/mud.bmp", true);
	floorTextureG = loader.loadTexture("res/Terrain/grassFlowers.bmp", true);
	floorTextureB = loader.loadTexture("res/Terrain/path.bmp", true);
	floorBlendMap = loader.loadTexture("res/Terrain/blendMap.bmp", true);

	//Load the texture and normalmap for the lamp model
	lampTexture = loader.loadTexture("res/Lantern/lantern.bmp", true);
	lampSpecularMap = loader.loadTexture("res/Lantern/lanternS.bmp", false);

	//Load the texture and normalmap for the barrel model
	barrelTexture = loader.loadTexture("res/Barrel/barrel.bmp", true);
	barrelNormal = loader.loadTexture("res/Barrel/barrelNormal.bmp", true);
}
// Load Safe Area Graphics
void LoadGraphics_SafeArea()
{
	// Load Building's textures.
	SA_T_Building[0] = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	SA_T_Building[1] = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	SA_T_Building[2] = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	SA_T_Building[3] = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	SA_T_Building[4] = loader.loadTexture("res/Terrain/grassy2.bmp", true);

	// Load AmmoBox's texture and normals.
	SA_T_AmmoBoxes[0]  = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	SA_T_AmmoBoxes[1]  = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	SA_TN_AmmoBoxes[0] = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	SA_TN_AmmoBoxes[1] = loader.loadTexture("res/Terrain/grassy2.bmp", true);

	// Load Barrel's texture.
	SA_T_Barrels = loader.loadTexture("res/Terrain/grassy2.bmp", true);

	// Load Barrier's texture and normals.
	SA_T_Barriers  = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	SA_TN_Barriers = loader.loadTexture("res/Terrain/grassy2.bmp", true);

	// Load Crate's texture and normal.
	SA_T_Crate  = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	SA_TN_Crate = loader.loadTexture("res/Terrain/grassy2.bmp", true);

	// Load Crate2's texture.
	SA_T_Crate2 = loader.loadTexture("res/Terrain/grassy2.bmp", true);

	SA_T_Pallets  = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	SA_TN_Pallets = loader.loadTexture("res/Terrain/grassy2.bmp", true);

	// Load SandBag's texture and normal.
	SA_T_SandBag  = loader.loadTexture("res/Terrain/grassy2.bmp", true);
	SA_TN_SandBag = loader.loadTexture("res/Terrain/grassy2.bmp", true);

	// Load SandBag2's texture.
	SA_T_SandBag2 = loader.loadTexture("res/Terrain/grassy2.bmp", true);
}

// Load all models
void loadModels()
{
	// ===  TERRAIN  ===
	//Create terrains with a heightmap, set position and all multitexture textures
	terrains[0].createWithHeightmap("res/heightmap.bmp", -1, -1, &loader, floorTextureGrass, floorTextureR, floorTextureG, floorTextureB, floorBlendMap);
	terrains[0].getModel()->setAmbientLight(0.2f);
	terrains[0].getModel()->setShadowMap(shadowRenderer.getShadowDepthTexture());

	terrains[1].createWithHeightmap("res/heightmap.bmp", 0, -1, &loader, floorTextureGrass, floorTextureR, floorTextureG, floorTextureB, floorBlendMap);
	terrains[1].getModel()->setAmbientLight(0.2f);
	terrains[1].getModel()->setShadowMap(shadowRenderer.getShadowDepthTexture());

	// ===  LAMP  ===
	// Load the lampModel
	gameobject lampModel = loader.loadObjFile("res/Lantern/lantern.obj", true, false);

	//Set lamp model and texture for all lamps
	for (int i = 0; i < 4; i++) {
		lamps[i].setModel(&lampModel);
		lamps[i].getModel()->addTexture(lampTexture);
		lamps[i].getModel()->setSpecularMap(lampSpecularMap);
	}

	//Initialise the models position, rotation and scale.
	lamps[0].getModel()->init(glm::vec3(20, terrains[1].getHeight(20, -30), -30), glm::vec3(0, 0, 0), glm::vec3(1.3f, 1.3f, 1.3f));
	lamps[1].getModel()->init(glm::vec3(370, terrains[1].getHeight(370, -300), -300), glm::vec3(0, 0, 0), glm::vec3(1.3f, 1.3f, 1.3f));
	lamps[2].getModel()->init(glm::vec3(293, terrains[1].getHeight(293, -305), -305), glm::vec3(0, 0, 0), glm::vec3(1.3f, 1.3f, 1.3f));
	lamps[3].getModel()->init(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1.3f, 1.3f, 1.3f));

	// ===  BARRELS  ===
	// Load barrelModel 1
	loadModel(model, "res/Barrel/barrel.obj",
		glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1.5f, 1.5f, 1.5f),
		barrelTexture, barrelNormal, shadowRenderer.getShadowDepthTexture(),
		100.0f, 1.0f, 0.1f);

	// Load barrelModel 2
	loadModel(model2, model,
		glm::vec3(20, terrains[1].getHeight(20, -10), -300), glm::vec3(0, 0, 0), glm::vec3(1.5f, 1.5f, 1.5f),
		barrelTexture, barrelNormal, shadowRenderer.getShadowDepthTexture(),
		100.0f, 1.0f, 0.1f);
}
// Load Safe Area Models
void loadModels_SafeArea()
{
	//TODO: Still need to change the 3DSMAX settings to OpenGL Settings
	// Load Building models.
	loadModel(SA_M_Building[0], "res/Barrel/barrel.obj", glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1.5f, 1.5f, 1.5f), SA_T_Building[0], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Building[1], "res/Barrel/barrel.obj", glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1.5f, 1.5f, 1.5f), SA_T_Building[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Building[2], "res/Barrel/barrel.obj", glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1.5f, 1.5f, 1.5f), SA_T_Building[2], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Building[3], "res/Barrel/barrel.obj", glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1.5f, 1.5f, 1.5f), SA_T_Building[3], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Building[4], "res/Barrel/barrel.obj", glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1.5f, 1.5f, 1.5f), SA_T_Building[4], 100.0f, 1.0f, 0.1f);

	// Load AmmoBoxes models.
	loadModel(SA_M_AmmoBoxes[0], "res/Barrel/barrel.obj", glm::vec3(-5.695f, -12.76f, 1.042f), glm::vec3(0, 0, glm::radians(133.379f)), glm::vec3(1.0f), SA_T_AmmoBoxes[0], SA_TN_AmmoBoxes[0], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[1], "res/Barrel/barrel.obj", glm::vec3(-5.695f, -12.76f, 1.042f), glm::vec3(0, 0, glm::radians(133.379f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[2], "res/Barrel/barrel.obj", glm::vec3(-5.695f, -12.76f, 1.042f), glm::vec3(0, 0, glm::radians(133.379f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[3], "res/Barrel/barrel.obj", glm::vec3(-5.695f, -12.76f, 1.042f), glm::vec3(0, 0, glm::radians(133.379f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[4], "res/Barrel/barrel.obj", glm::vec3(-5.695f, -12.76f, 1.042f), glm::vec3(0, 0, glm::radians(133.379f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[5], "res/Barrel/barrel.obj", glm::vec3(-5.695f, -12.76f, 1.042f), glm::vec3(0, 0, glm::radians(133.379f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[6], "res/Barrel/barrel.obj", glm::vec3(-5.695f, -12.76f, 1.042f), glm::vec3(0, 0, glm::radians(133.379f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	
	loadModel(SA_M_AmmoBoxes[7],  SA_M_AmmoBoxes[0], glm::vec3(3.656f, 13.91f, 1.042f), glm::vec3(0, 0, glm::radians(79.083f)), glm::vec3(1.0f), SA_T_AmmoBoxes[0], SA_TN_AmmoBoxes[0], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[8],  SA_M_AmmoBoxes[1], glm::vec3(3.656f, 13.91f, 1.042f), glm::vec3(0, 0, glm::radians(79.083f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[9],  SA_M_AmmoBoxes[2], glm::vec3(3.656f, 13.91f, 1.042f), glm::vec3(0, 0, glm::radians(79.083f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[10], SA_M_AmmoBoxes[3], glm::vec3(3.656f, 13.91f, 1.042f), glm::vec3(0, 0, glm::radians(79.083f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[11], SA_M_AmmoBoxes[4], glm::vec3(3.656f, 13.91f, 1.042f), glm::vec3(0, 0, glm::radians(79.083f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[12], SA_M_AmmoBoxes[5], glm::vec3(3.656f, 13.91f, 1.042f), glm::vec3(0, 0, glm::radians(79.083f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[13], SA_M_AmmoBoxes[6], glm::vec3(3.656f, 13.91f, 1.042f), glm::vec3(0, 0, glm::radians(79.083f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	
	loadModel(SA_M_AmmoBoxes[14], SA_M_AmmoBoxes[0], glm::vec3(-7.26f, -2.125f, 1.042f), glm::vec3(0, 0, glm::radians(-178.774f)), glm::vec3(1.0f), SA_T_AmmoBoxes[0], SA_TN_AmmoBoxes[0], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[15], SA_M_AmmoBoxes[1], glm::vec3(-7.26f, -2.125f, 1.042f), glm::vec3(0, 0, glm::radians(-178.774f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[16], SA_M_AmmoBoxes[2], glm::vec3(-7.26f, -2.125f, 1.042f), glm::vec3(0, 0, glm::radians(-178.774f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[17], SA_M_AmmoBoxes[3], glm::vec3(-7.26f, -2.125f, 1.042f), glm::vec3(0, 0, glm::radians(-178.774f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[18], SA_M_AmmoBoxes[4], glm::vec3(-7.26f, -2.125f, 1.042f), glm::vec3(0, 0, glm::radians(-178.774f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[19], SA_M_AmmoBoxes[5], glm::vec3(-7.26f, -2.125f, 1.042f), glm::vec3(0, 0, glm::radians(-178.774f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_AmmoBoxes[20], SA_M_AmmoBoxes[6], glm::vec3(-7.26f, -2.125f, 1.042f), glm::vec3(0, 0, glm::radians(-178.774f)), glm::vec3(1.0f), SA_T_AmmoBoxes[1], SA_TN_AmmoBoxes[1], 100.0f, 1.0f, 0.1f);

	// Load Barrels models.
	loadModel(SA_M_Barrels[0], "res/Barrel/barrel.obj", glm::vec3(-7.349f, 8.582f, 0.513f), glm::vec3(0, 0, glm::radians(-22.889f)), glm::vec3(1.0f), SA_T_Barrels, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Barrels[1], SA_M_Barrels[0], glm::vec3(-4.407f, 13.685f, 0.893f), glm::vec3(glm::radians(14.743f), glm::radians(0.389f), glm::radians(-91.479f)), glm::vec3(1.0f), SA_T_Barrels, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Barrels[2], SA_M_Barrels[0], glm::vec3(-7.349f, 12.441f, 3.818f), glm::vec3(glm::radians(-5.954f), 0, 0), glm::vec3(1.0f), SA_T_Barrels, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Barrels[3], SA_M_Barrels[0], glm::vec3(-7.349f, 13.585f, 0.513f), glm::vec3(0.0f), glm::vec3(1.0f), SA_T_Barrels, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Barrels[4], SA_M_Barrels[0], glm::vec3(-7.349f, 10.777f, 0.513f), glm::vec3(glm::radians(-16.803f), glm::radians(-3.727f), glm::radians(20.35f)), glm::vec3(1.0f), SA_T_Barrels, 100.0f, 1.0f, 0.1f);
	
	// Load Barriers models.
	loadModel(SA_M_Barriers[0], "res/Barrel/barrel.obj", glm::vec3(5.746f, -14.737f, 1.817f), glm::vec3(0.0f), glm::vec3(1.0f), SA_T_Barriers, SA_TN_Barriers, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Barriers[1], SA_M_Barriers[0], glm::vec3(2.48f, -14.737f, 1.817f), glm::vec3(0.0f), glm::vec3(1.0f), SA_T_Barriers, SA_TN_Barriers, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Barriers[2], SA_M_Barriers[0], glm::vec3(-0.987f, -14.737f, 1.817f), glm::vec3(0.0f), glm::vec3(1.0f), SA_T_Barriers, SA_TN_Barriers, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Barriers[3], SA_M_Barriers[0], glm::vec3(-4.258f, -14.737f, 1.817f), glm::vec3(0.0f), glm::vec3(1.0f), SA_T_Barriers, SA_TN_Barriers, 100.0f, 1.0f, 0.1f);

	// Load Crates models.
	loadModel(SA_M_Crate[0], "res/Barrel/barrel.obj", glm::vec3(9.514f, 8.295f, 4.408f), glm::vec3(0, 0, glm::radians(-18.961f)), glm::vec3(1.0f), SA_T_Crate, SA_TN_Crate, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Crate[1], SA_M_Crate[0], glm::vec3(8.96f, 9.678f, 1.864f), glm::vec3(0, 0, 0), glm::vec3(1.0f), SA_T_Crate, SA_TN_Crate, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Crate[2], SA_M_Crate[0], glm::vec3(9.385f, -4.265f, 2.049f), glm::vec3(glm::radians(-0.109f), glm::radians(-15.49f), glm::radians(0.046f)), glm::vec3(1.0f), SA_T_Crate, SA_TN_Crate, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Crate[3], SA_M_Crate[0], glm::vec3(9.552f, 5.291f, 4.424f), glm::vec3(0, 0, 0), glm::vec3(1.0f), SA_T_Crate, SA_TN_Crate, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Crate[4], SA_M_Crate[0], glm::vec3(9.552f, -4.021f, 4.725f), glm::vec3(0, glm::radians(-5.235f), 0), glm::vec3(1.0f), SA_T_Crate, SA_TN_Crate, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Crate[5], SA_M_Crate[0], glm::vec3(8.96f, 5.932f, 1.864), glm::vec3(0, 0, 0), glm::vec3(1.0f), SA_T_Crate, SA_TN_Crate, 100.0f, 1.0f, 0.1f);
	
	// Load Crate2 model.
	loadModel(SA_M_Crate2, "res/Barrel/barrel.obj", glm::vec3(10.1f, 6.945f, 6.599), glm::vec3(0, 0, glm::radians(6.99f)), glm::vec3(1.0f), SA_T_Crate2, 100.0f, 1.0f, 0.1f);
	
	// Load Pallets models.
	loadModel(SA_M_Pallets[0], "res/Barrel/barrel.obj", glm::vec3(9.958f, 12.077f, 2.466f), glm::vec3(glm::radians(90.0f), glm::radians(-81.467f), glm::radians(29.358)), glm::vec3(1.0f), SA_T_Pallets, SA_TN_Pallets, 100.0f, 1.0f, 0.1f);
	loadModel(SA_M_Pallets[1], SA_M_Pallets[0], glm::vec3(9.362f, 14.006f, 2.466f), glm::vec3(glm::radians(90.0f), glm::radians(-81.467f), glm::radians(52.264f)), glm::vec3(1.0f), SA_T_Pallets, SA_TN_Pallets, 100.0f, 1.0f, 0.1f);
	
	// Load SandBag model.
	loadModel(SA_M_SandBag, "res/Barrel/barrel.obj", glm::vec3(8.933f, -10.087f, 0.513f), glm::vec3(0, 0, glm::radians(66.783f)), glm::vec3(1.0f), SA_T_SandBag, SA_TN_SandBag, 100.0f, 1.0f, 0.1f);

	// Load SandBag2 model.
	loadModel(SA_M_SandBag2, "res/Barrel/barrel.obj", glm::vec3(-8.35f, -6.885f, -2.656f), glm::vec3(glm::radians(-1.528f), glm::radians(-17.504f), glm::radians(-145.947f)), glm::vec3(1.0f), SA_T_SandBag2, 100.0f, 1.0f, 0.1f);

}


//  =========  Initialise a model  ===========

// Load a model with only a texture
void loadModel(Model &model, // Variable to set
	std::string modelFilename, // Model filename
	glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, // Initial Position, rotation and scale
	GLuint textureID,  // Textures
	float shineDamper, float reflectivity,
	float ambientLight) // Reflectivity
{
	// Load modeldata and set the variable.
	model.setModel(&gameobject(loader.loadObjFile(modelFilename.c_str(), false, false)));
	// Initialise model
	model.Initialise(pos, rot, scale);

	// Add a texture
	model.getModel()->addTexture(textureID);

	// Set the shineDamper
	model.getModel()->setShineDamper(shineDamper);
	// Set the reflectivity
	model.getModel()->setReflectivity(reflectivity);

	// Set the ambientLight
	model.getModel()->setAmbientLight(ambientLight);
}
// Load a model with a texture and normalMap
void loadModel(Model &model, // Variable to set
	std::string modelFilename, // Model filename
	glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, // Initial Position, rotation and scale
	GLuint textureID, GLuint normalTextureID, // Textures
	float shineDamper, float reflectivity,
	float ambientLight) // Reflectivity
{
	loadModel(model, modelFilename, pos, rot, scale, textureID, shineDamper, reflectivity, ambientLight);

	// Set the normalmap
	model.getModel()->setNormalMap(normalTextureID);
}
// Load a model with a texture, normalMap and a shadowMap
void loadModel(Model &model, // Variable to set
	std::string modelFilename, // Model filename
	glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, // Initial Position, rotation and scale
	GLuint textureID, GLuint normalTextureID, GLuint shadowMapID, // Textures
	float shineDamper, float reflectivity,
	float ambientLight)
{
	// Load the model
	loadModel(model, modelFilename, pos, rot, scale, textureID, normalTextureID, shineDamper, reflectivity, ambientLight);

	// Set the shadowmap
	model.getModel()->setShadowMap(shadowMapID);
}

// Set a model with only a texture
void loadModel(Model &model, Model &oriModel, // Variable to set
	glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, // Initial Position, rotation and scale
	GLuint textureID,  // Textures
	float shineDamper, float reflectivity,
	float ambientLight) // Reflectivity
{
	model.setModel(oriModel.getModel());

	// Initialise model
	model.Initialise(pos, rot, scale);

	// Add a texture
	model.getModel()->addTexture(textureID);

	// Set the shineDamper
	model.getModel()->setShineDamper(shineDamper);
	// Set the reflectivity
	model.getModel()->setReflectivity(reflectivity);

	// Set the ambientLight
	model.getModel()->setAmbientLight(ambientLight);
}
// Set a model with a texture and normalMap
void loadModel(Model &model, Model &oriModel,// Variable to set
	glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, // Initial Position, rotation and scale
	GLuint textureID, GLuint normalTextureID, // Textures
	float shineDamper, float reflectivity,
	float ambientLight) // Reflectivity
{
	loadModel(model, oriModel, pos, rot, scale, textureID, shineDamper, reflectivity, ambientLight);

	// Set the normalmap
	model.getModel()->setNormalMap(normalTextureID);
}
// Set a model with a texture, normalMap and a shadowMap
void loadModel(Model &model, Model &oriModel,// Variable to set
	glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, // Initial Position, rotation and scale
	GLuint textureID, GLuint normalTextureID, GLuint shadowMapID, // Textures
	float shineDamper, float reflectivity,
	float ambientLight)
{
	// Load the model
	loadModel(model, oriModel, pos, rot, scale, textureID, normalTextureID, shineDamper, reflectivity, ambientLight);

	// Set the shadowmap
	model.getModel()->setShadowMap(shadowMapID);
}