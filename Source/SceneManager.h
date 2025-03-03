///////////////////////////////////////////////////////////////////////////////
// shadermanager.h
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "ShapeMeshes.h"

#include <string>
#include <vector>

/***********************************************************
 *  SceneManager
 *
 *  This class contains the code for preparing and rendering
 *  3D scenes, including the shader settings.
 ***********************************************************/
class SceneManager
{
public:
	// constructor
	SceneManager(ShaderManager *pShaderManager);
	// destructor
	~SceneManager();

	struct TEXTURE_INFO
	{
		std::string tag;
		uint32_t ID;
	};

	struct OBJECT_MATERIAL
	{
		float ambientStrength;
		glm::vec3 ambientColor;
		glm::vec3 diffuseColor;
		glm::vec3 specularColor;
		float shininess;
		std::string tag;
	};

private:
	// pointer to shader manager object
	ShaderManager* m_pShaderManager;
	// pointer to basic shapes object
	ShapeMeshes* m_basicMeshes;
	// total number of loaded textures
	int m_loadedTextures;
	// loaded textures info
	TEXTURE_INFO m_textureIDs[16];
	// defined object materials
	std::vector<OBJECT_MATERIAL> m_objectMaterials;

	// load texture images and convert to OpenGL texture data
	bool CreateGLTexture(const char* filename, std::string tag);
	// bind loaded OpenGL textures to slots in memory
	void BindGLTextures();
	// free the loaded OpenGL textures
	void DestroyGLTextures();
	// find a loaded texture by tag
	int FindTextureID(std::string tag);
	int FindTextureSlot(std::string tag);
	// find a defined material by tag
	bool FindMaterial(std::string tag, OBJECT_MATERIAL& material);

	// set the transformation values 
	// into the transform buffer
	void SetTransformations(
		glm::vec3 scaleXYZ,
		float XrotationDegrees,
		float YrotationDegrees,
		float ZrotationDegrees,
		glm::vec3 positionXYZ);

	// set the color values into the shader
	void SetShaderColor(
		float redColorValue,
		float greenColorValue,
		float blueColorValue,
		float alphaValue);

	// set the texture data into the shader
	void SetShaderTexture(
		std::string textureTag);

	// set the UV scale for the texture mapping
	void SetTextureUVScale(
		float u, float v);

	// set the object material into the shader
	void SetShaderMaterial(
		std::string materialTag);

public:

	//Loads the textures for the scene
	void LoadSceneTextures();
	//Loads meshes and lights
	void PrepareScene();
	//Renders the basic scene by calling the other render functions
	void RenderScene();
	//Light defining function
	void SetupSceneLights();
	//Object material definition
	void DefineObjectMaterials();
	//Renders the main floor
	void RenderFloor();
	//Renders the four side walls
	void RenderWalls();
	//Renders All walls that divide the quadrants
	void RenderQuadrantWalls();
	//Renders All the objects in quadrant one
	void RenderQuadrantOne();
	//Renders all the objects in quadrant two
	void RenderQuadrantTwo();
	//Renders all the objects in quadrant three
	void RenderQuadrantThree();
	//Renders all the objects in quadrant four
	void RenderQuadrantFour();
	//Adds a root for a short bush at each x,z coordinate
	//helper function for RenderQuadrantTwo
	void AddRoot(float x, float z);

};