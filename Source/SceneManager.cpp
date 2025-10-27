///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{                
	CreateGLTexture("textures/tableWood.jpg", "woodTexture");
	CreateGLTexture("textures/glassTop.jpg", "glassTexture");
	CreateGLTexture("textures/glassBottom.jpg", "glassTexture2");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures

	BindGLTextures();
}

 /***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
// Cheese
	OBJECT_MATERIAL cheese;
	cheese.tag = "cheese";
	cheese.diffuseColor = glm::vec3(1.0f, 0.85f, 0.3f);
	cheese.specularColor = glm::vec3(0.9f, 0.8f, 0.4f);
	cheese.shininess = 32.0f;
	m_objectMaterials.push_back(cheese);

	// Grapes
	OBJECT_MATERIAL grapes;
	grapes.tag = "grapes";
	grapes.diffuseColor = glm::vec3(0.6f, 0.1f, 0.6f);
	grapes.specularColor = glm::vec3(0.8f, 0.2f, 0.8f);
	grapes.shininess = 32.0f;
	m_objectMaterials.push_back(grapes);

	// Cherries
	OBJECT_MATERIAL cherries;
	cherries.tag = "cherries";
	cherries.diffuseColor = glm::vec3(1.0f, 0.0f, 0.0f);
	cherries.specularColor = glm::vec3(0.9f, 0.1f, 0.1f);
	cherries.shininess = 32.0f;
	m_objectMaterials.push_back(cherries);

	// Crackers
	OBJECT_MATERIAL crackers;
	crackers.tag = "crackers";
	crackers.diffuseColor = glm::vec3(0.9f, 0.75f, 0.5f);
	crackers.specularColor = glm::vec3(0.7f, 0.65f, 0.5f);
	crackers.shininess = 8.0f;
	m_objectMaterials.push_back(crackers);

	// Glass
	OBJECT_MATERIAL glass;
	glass.tag = "glass";
	glass.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glass.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glass.shininess = 500.0f;
	m_objectMaterials.push_back(glass);

	// Wood
	OBJECT_MATERIAL wood;
	wood.tag = "wood";
	wood.diffuseColor = glm::vec3(0.7f, 0.45f, 0.2f); 
	wood.specularColor = glm::vec3(0.3f, 0.2f, 0.1f);
	wood.shininess = 16.0f;
	m_objectMaterials.push_back(wood);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Directional Light - main sunlight
	m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.5f, -1.0f, -0.3f)); 
	m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.35f, 0.35f, 0.35f)); 
	m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(1.0f, 0.92f, 0.75f)); 
	m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(1.0f, 1.0f, 1.0f)); 
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point Light - soft colored accent
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(0.25f, 0.25f, 0.15f));
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.05f, 0.02f, 0.03f));
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(1.0f, 0.6f, 0.45f)); 
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(1.0f, 0.7f, 0.5f)); 
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	//define the materials for objects in the scene
	DefineObjectMaterials();
	//add and define the light sources for the scene
	SetupSceneLights();
	// load the textures for the 3D scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadBoxMesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	/**********************
	 * Board 
	 **********************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.02f, 0.65f); 

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ, 
		XrotationDegrees,
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ);

	// set the texture for the next draw command
	SetShaderTexture("woodTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	// draw the plane mesh
	m_basicMeshes->DrawPlaneMesh();

	/**********************
	 * Cheese Slices Cluster
	 **********************/

	// set the XYZ positions for the mesh
	glm::vec3 cheesePositions[] = {
		glm::vec3(0.2f, 0.02f, 0.15f),
		glm::vec3(0.25f, 0.02f, 0.18f),
		glm::vec3(0.28f, 0.02f, 0.12f)
	};

	// set the XYZ rotations for the mesh
	glm::vec3 cheeseRotations[] = {
		glm::vec3(0.0f, 0.0f, 5.0f),
		glm::vec3(0.0f, 10.0f, 0.0f),
		glm::vec3(0.0f, 15.0f, -5.0f)
	};

	// set the rotation and shader for each cheese x3
	for (int i = 0; i < 3; i++) {
		scaleXYZ = glm::vec3(0.1f, 0.01f, 0.05f);
		XrotationDegrees = cheeseRotations[i].x;
		YrotationDegrees = cheeseRotations[i].y;
		ZrotationDegrees = cheeseRotations[i].z;
		positionXYZ = cheesePositions[i];
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("cheese");
		m_basicMeshes->DrawBoxMesh();
	}

	/**********************
	 * Grapes Cluster
	 **********************/

	// set the positions for the mesh
	glm::vec3 grapePositions[] = {
		glm::vec3(0.0f, 0.03f, 0.2f),
		glm::vec3(0.025f, 0.03f, 0.215f),
		glm::vec3(-0.02f, 0.03f, 0.185f),
		glm::vec3(0.015f, 0.03f, 0.18f)
	};

	// set the rotation and shaders for each grape x4
	for (int i = 0; i < 4; i++) {
		scaleXYZ = glm::vec3(0.02f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = grapePositions[i];
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("grapes");
		m_basicMeshes->DrawSphereMesh();
	}

	/**********************
	 * Cherries Cluster
	 **********************/

	// set the positions for the mesh
	glm::vec3 cherryPositions[] = {
		glm::vec3(-0.1f, 0.03f, -0.05f),
		glm::vec3(-0.08f, 0.03f, -0.07f),
		glm::vec3(-0.115f, 0.03f, -0.045f)
	};

	// set the rotation and shaders for each cherry x3
	for (int i = 0; i < 3; i++) {
		scaleXYZ = glm::vec3(0.02f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = cherryPositions[i];
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("cherries");
		m_basicMeshes->DrawSphereMesh();
	}

	/**********************
	 * Crackers Cluster
	 **********************/

	// set the positions for the mesh
	glm::vec3 crackerPositions[] = {
		glm::vec3(-0.2f, 0.025f, 0.1f),
		glm::vec3(-0.23f, 0.025f, 0.14f),
		glm::vec3(-0.18f, 0.025f, 0.07f)
	};

	// set the rotations and shaders for each cracker x3
	glm::vec3 crackerRotations[] = {
		glm::vec3(0.0f, 0.0f, 10.0f),
		glm::vec3(0.0f, 5.0f, -5.0f),
		glm::vec3(0.0f, -10.0f, 15.0f)
	};
	for (int i = 0; i < 3; i++) {
		scaleXYZ = glm::vec3(0.05f, 0.01f, 0.05f);
		XrotationDegrees = crackerRotations[i].x;
		YrotationDegrees = crackerRotations[i].y;
		ZrotationDegrees = crackerRotations[i].z;
		positionXYZ = crackerPositions[i];
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("crackers");
		m_basicMeshes->DrawCylinderMesh();
	}


	/**********************
	 * Wine Glass Base
	 **********************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.02f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.3f, 0.02f, -0.15f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set shaders for next draw command
	SetShaderColor(0.7f, 0.85f, 0.9f, 0.4f);
	SetShaderMaterial("glass");
	SetShaderTexture("glassTexture");
	SetTextureUVScale(1.0f, 1.0f);

	// draw cylinder mesh
	m_basicMeshes->DrawCylinderMesh();

	/**********************
	 * Wine Glass Stem
	 **********************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.15f, 0.03f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.3f, 0.05f, -0.15f);

	// set the transformations for the mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the shaders for the mesh
	SetShaderColor(0.7f, 0.85f, 0.9f, 0.4f);
	SetShaderMaterial("glass");

	// draw cyclinder mesh
	m_basicMeshes->DrawCylinderMesh();

	/**********************
	 * Wine Glass Cup
	 **********************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.12f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.3f, 0.2f, -0.15f);

	// set the transformations for the mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the shaders for the mesh
	SetShaderColor(0.7f, 0.85f, 0.9f, 0.4f);
	SetShaderTexture("glassTexture2");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("glass");

	// draw cylinder mesh
	m_basicMeshes->DrawCylinderMesh();
}
	


