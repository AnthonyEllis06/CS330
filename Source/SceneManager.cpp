///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
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
		std::cout << "failed here";
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/// <summary>
/// Loads the images and converts them to textures.
/// </summary>
void SceneManager::LoadSceneTextures() 
{
	bool bReturn = false;
	stbi_set_flip_vertically_on_load(true);
	// load the texture images and convert to OpenGL texture data		
	bReturn = CreateGLTexture("../../Utilities/Textures/BushDenseBerries.jpg", "DenseBerries");
	bReturn = CreateGLTexture("../../Utilities/Textures/bushDense.jpg", "Hedge");
	bReturn = CreateGLTexture("../../Utilities/Textures/BarkTexture.jpg", "bark");
	bReturn = CreateGLTexture("../../Utilities/Textures/pavers.jpg", "brick");
	bReturn = CreateGLTexture("../../Utilities/Textures/tilesf2.jpg", "tile");
	bReturn = CreateGLTexture("../../Utilities/Textures/rocks.jpg", "rocks");
	
	BindGLTextures();
}

/// <summary>
/// Creates the object shader material
/// </summary>
void SceneManager::DefineObjectMaterials()
{

	//Cement Type of material or dark material
	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.6f);
	cementMaterial.ambientStrength = 0.0f;
	cementMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cementMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cementMaterial.shininess = 0.1;
	cementMaterial.tag = "cement";

	m_objectMaterials.push_back(cementMaterial);

	//Tile material with a tint of blue
	OBJECT_MATERIAL blueTile;
	blueTile.ambientColor = glm::vec3(0.1f, 0.1f, 0.6f);
	blueTile.ambientStrength = 0.1f;
	blueTile.diffuseColor = glm::vec3(0.1f, 0.1f, 0.3f);
	blueTile.specularColor = glm::vec3(0.1f, 0.1f, 0.3f);
	blueTile.shininess = 0.6f;
	blueTile.tag = "blueTile";

	m_objectMaterials.push_back(blueTile);

	//bush material with a tint of green
	OBJECT_MATERIAL bush;
	bush.ambientColor = glm::vec3(0.1f, 0.8f, 0.1f);
	bush.ambientStrength = 0.1f;
	bush.diffuseColor = glm::vec3(0.1f, 0.8, 0.1f);
	bush.specularColor = glm::vec3(0.1f, 0.8, 0.1f);
	bush.shininess = 0.1f;
	bush.tag = "bush";

	m_objectMaterials.push_back(bush);

	//bark material with a mixed tint
	OBJECT_MATERIAL bark;
	bark.ambientColor = glm::vec3(0.5f, 0.5f, 0.1f);
	bark.ambientStrength = 0.1f;
	bark.diffuseColor = glm::vec3(0.5f, 0.5, 0.1f);
	bark.specularColor = glm::vec3(0.5f, 0.5, 0.1f);
	bark.shininess = 0.1f;
	bark.tag = "bark";

	m_objectMaterials.push_back(bark);

}

/// <summary>
/// Defines and sets scene lights
/// </summary>
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	//Light one                             
	m_pShaderManager->setVec3Value("lightSources[0].positon", 0.0f, 11.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].ambientStrength", 0.6f);

	//Light two                             
	m_pShaderManager->setVec3Value("lightSources[1].positon", -50.0f, 11.0f, -50.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.8f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.8f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.8f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 0.5f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.5f);
	m_pShaderManager->setFloatValue("lightSources[1].ambientStrength", 0.6f);

	//Light three
	m_pShaderManager->setVec3Value("lightSources[2].positon", 50.0f, 11.0f,50.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.1f, 0.1f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.1f, 0.1f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.1f, 0.1f, 0.8f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 0.6f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.6f);
	m_pShaderManager->setFloatValue("lightSources[2].ambientStrength", 0.6f);

	//Light four
	m_pShaderManager->setVec3Value("lightSources[3].positon", 50.0f, 11.0f, -50.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.1f, 0.7f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.1f, 0.7f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.0f, 0.7f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].ambientStrength", 1.0f);


	//Light one                            



	

	m_pShaderManager->setBoolValue("bUseLightng", true);

}

/// <summary>
/// Loads the object meshes and prepares scene lights and textures
/// </summary>
void SceneManager::PrepareScene()
{
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	LoadSceneTextures();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadPyramid4Mesh();
}

/// <summary>
/// The full render function
/// Calls all the individual render functions
/// </summary>
void SceneManager::RenderScene()
{
	RenderFloor();
	RenderWalls();
	RenderQuadrantWalls();
	RenderQuadrantOne();
	RenderQuadrantTwo();
	RenderQuadrantThree();
	RenderQuadrantFour();


}

/// <summary>
/// Renders and defines the floor
/// </summary>
void SceneManager::RenderFloor() 
{
	//FLOOR
	/******************************************************************/
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
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(100.0f, 1.0f, 100.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -20.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("brick");
	SetShaderMaterial("cement");
	SetTextureUVScale(20, 20);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	
}

/// <summary>
/// Renders all the objects in quadrant one
/// </summary>
void SceneManager::RenderQuadrantOne()
{
	//QUADRANT 1
	/******************************************************************/
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	/******************************************************************/


	//CENTER BLOCK	
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 0.5f, 20.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(55.0f, 0.5f, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("tile");
	SetShaderMaterial("blueTile");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/ 

	//CENTER MULCH
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(19.0f, 0.5f, 19.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(55.0f, 0.6f, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("rocks");
	SetShaderMaterial("cement");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	//LEFT BOTTOM CONE
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(50.0f, 0.9f, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//LEFT TOP CONE
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(50.0f, 2.9f, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//RIGHT BOTTOM CONE
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(60.0f, 0.9f, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//RIGHT TOP CONE
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(60.0f, 2.9f, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//RIGHT TORUS
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 5.0f, 3.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(60.0f, 8.8f, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(3, 2);
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/******************************************************************/

	//LEFT TORUS
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 5.0f, 3.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(50.0f, 8.8f, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(3, 2);
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/******************************************************************/

	//CENTER TORUS
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 5.0f, 3.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(55.0f, 18.5f, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(3, 2);
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/******************************************************************/



}

/// <summary>
/// Renders all the small bushes in quadrant two
/// uses helper function add root, which defines the root height also
/// </summary>
void SceneManager::RenderQuadrantTwo() 
{
	//QUADRANT 2
	/******************************************************************/
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	/******************************************************************/

	//Usefull for moveving all spheres
	float sHeight = 3.3;
	float sSize = 1.3;

	//CENTER BLOCK	
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 0.5f, 20.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-55.0f, 0.5f, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("tile");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	//CENTER MULCH
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(19.0f, 0.5f, 19.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-55.0f, 0.6f, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("rocks");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	//Sphere 1
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-48.0f, sHeight, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-48.0f, 35.0f);
	/******************************************************************/

	//Sphere 2
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-62.0f, sHeight, 35.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-62.0f, 35.0f);
	/******************************************************************/

	//Sphere three
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-55.0f, sHeight, 42.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	AddRoot(-55.0f, 42.0f);
	/******************************************************************/

	//Sphere 4
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-55.0f, sHeight, 28.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-55.0f, 28.0f);
	/******************************************************************/

	//Sphere 5
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-50.0f, sHeight, 40.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-50.0f, 40.0f);
	/******************************************************************/

	//Sphere 6
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-60.0f, sHeight, 30.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-60.0f, 30.0f);
	/******************************************************************/

	//Sphere 7
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-60.0f, sHeight, 40.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-60.0f, 40.0f);
	/******************************************************************/

	//Sphere 8
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-50.0f, sHeight, 30.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-50.0f, 30.0f);
	/******************************************************************/

	//Sphere 9
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-52.3f, sHeight, 28.3f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-52.3f, 28.3f);
	/******************************************************************/

	//Sphere 10
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-57.7f, sHeight, 41.7f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-57.7f, 41.7f);
	/******************************************************************/

	//Sphere 11
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-57.7f, sHeight, 28.3f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-57.7f, 28.3f);
	/******************************************************************/

	//Sphere 12
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-52.3f, sHeight, 41.7f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-52.3f, 41.7f);
	/******************************************************************/

	//Sphere 13
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-61.7f, sHeight, 37.7f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-61.7f, 37.7f);
	/******************************************************************/

	//Sphere 14
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-48.3f, sHeight, 32.3f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-48.3f, 32.3f);
	/******************************************************************/

	//Sphere 15
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-61.7f, sHeight, 32.3f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-61.7f, 32.3f);
	/******************************************************************/

	//Sphere 16
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(sSize, sSize, sSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-48.3f, sHeight, 37.7f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	AddRoot(-48.3f, 37.7f);
	/******************************************************************/


}

/// <summary>
/// Renders all the objects in quadrant three
/// </summary>
void SceneManager::RenderQuadrantThree()
{
	//QUADRANT 3
	/******************************************************************/
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	/******************************************************************/


	//CENTER BLOCK	
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 0.5f, 20.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(55.0f, 0.5f, -75.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("tile");
	SetShaderMaterial("blueTile");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	//CENTER MULCH
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(19.0f, 0.5f, 19.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(55.0f, 0.6f, -75.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("rocks");
	SetShaderMaterial("cement");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	//BOTTOM CONE
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(55.0f, 0.9f, -75.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//TOP CONE
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(55.0f, 2.9f, -75.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//bottom square
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 4.0f, 4.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(55.0f, 4.9f, -75.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	//sphere
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 2.5f, 2.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(55.0f, 9.3f, -75.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	//bottom square
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 4.0f, 4.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(55.0f, 13.5f, -75.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();
	/******************************************************************/


}

/// <summary>
/// Renders all the objects in quadrant four
/// </summary>
void SceneManager::RenderQuadrantFour()
{
	//QUADRANT 4
	/******************************************************************/
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	/******************************************************************/

	//Useful variables for this quadrant
	float pSize = 8.5f;
	float pHeight = 6.7f;

	//CENTER BLOCK	
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 0.5f, 20.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-55.0f, 0.5f, -75.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("tile");
	SetShaderMaterial("blueTile");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	//CENTER MULCH
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(19.0f, 0.5f, 19.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-55.0f, 0.6f, -75.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("rocks");
	SetShaderMaterial("cement");
	SetTextureUVScale(10, 10);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	//cone one bottom
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-50.0f, 0.6f, -70.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//cone one top
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.5f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-50.0f, 2.6f, -70.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//Pyramid one
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(pSize, pSize, pSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-50.0f, pHeight, -70.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();
	/******************************************************************/

	//cone two bottom
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-60.0f, 0.6f, -80.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//cone two top
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.5f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-60.0f, 2.6f, -80.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//Pyramid two
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(pSize, pSize, pSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-60.0f, pHeight, -80.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();
	/******************************************************************/

	//cone three bottom
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-60.0f, 0.6f, -70.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//cone three top
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.5f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-60.0f, 2.6f, -70.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//Pyramid three
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(pSize, pSize, pSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-60.0f, pHeight, -70.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();
	/******************************************************************/

	//cone four bottom
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-50.0f, 0.6f, -80.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//cone four top
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.5f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-50.0f, 2.6f, -80.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//Pyramid four
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(pSize, pSize, pSize);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-50.0f, pHeight, -80.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();
	/******************************************************************/

	//Pyramid top
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(11.0f, 11.0f, 11.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-55.0f, 16.0f, -75.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Hedge");
	SetShaderMaterial("bush");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();
	/******************************************************************/




}

/// <summary>
/// Renders the divind walls between the quadrants
/// </summary>
void SceneManager::RenderQuadrantWalls() 
{
	//QUADRANT WALLS
	
	// declare the variables for the transformations
	/******************************************************************/
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	/******************************************************************/

	//FRONT DIVIDER
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 20.0f, 50.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, 50.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("DenseBerries");
	SetTextureUVScale(10, 5);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//BACK DIVIDER
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 20.0f, 50.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -90.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("DenseBerries");
	SetTextureUVScale(10, 5);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//LEFT DIVIDER
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 20.0f, 50.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-70.0f, 10.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("DenseBerries");
	SetTextureUVScale(10, 5);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//RIGHT DIVIDER
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 20.0f, 50.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(70.0f, 10.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("DenseBerries");
	SetTextureUVScale(10, 5);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}

/// <summary>
/// Renders the outer border walls
/// </summary>
void SceneManager::RenderWalls() 
{
	//WALLS
	/******************************************************************/
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/

	// BACK WALL
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 20.0f, 200.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -117.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("DenseBerries");
	SetShaderMaterial("bush");
	SetTextureUVScale(10, 5);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// LEFT WALL
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 20.0f, 200.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-97.5f, 10.0f, -20.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// RIGHT WALL
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 20.0f, 200.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(97.5f, 10.0f, -20.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// FRONT WALL
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 20.0f, 200.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, 77.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

}

/// <summary>
/// Add Root renders a small root for the desired location in the x, z plane.
/// </summary>
/// <param name="x">is the x coordinate in the plane</param>
/// <param name="z">is the z coordinate in the plane</param>
void SceneManager::AddRoot(float x, float z) 
{
	/******************************************************************/
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	/******************************************************************/

	//LEFT BOTTOM CONE
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(x, 0.9f, z);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/

	//TOP CONE
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(x, 2.9f, z);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("bark");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/******************************************************************/
}