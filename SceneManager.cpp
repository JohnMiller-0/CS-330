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
	const char* g_Texture2 = "texture2";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
	const char* g_BlendTexture = "bBlendTexture";
	const char* g_MixFactor = "bMixFactor";
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

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  LoadGLTextures()
 *
 *  This method is used for passing tags and file paths to 
 *  texture creators
 ***********************************************************/
void SceneManager::LoadGLTextures()
{

	bool bReturn = false;
	bReturn = CreateGLTexture(
		"../7-1_FinalProjectMilestones/textures/tableCloth.jpg",
		"cloth");

	bReturn = CreateGLTexture(
		"../7-1_FinalProjectMilestones/textures/bottleLid.jpg",
		"bottleLid");

	bReturn = CreateGLTexture(
		"../7-1_FinalProjectMilestones/textures/bread.jpg",
		"breadTop");

	bReturn = CreateGLTexture(
		"../7-1_FinalProjectMilestones/textures/butter.jpg",
		"butter"); 
	
	bReturn = CreateGLTexture(
		"../7-1_FinalProjectMilestones/textures/cracks.jpg",
		"cracks");

	bReturn = CreateGLTexture(
		"../7-1_FinalProjectMilestones/textures/ORANGE.jpg",
		"orange");
	
	bReturn = CreateGLTexture(
		"../7-1_FinalProjectMilestones/textures/side.jpg",
		"breadSide");
	
	bReturn = CreateMirroredTexture(
		"../7-1_FinalProjectMilestones/textures/Untitled_Artwork.jpg",
		"skull");

	bReturn = CreateGLTexture(
		"../7-1_FinalProjectMilestones/textures/wall.jpg",
		"wall");

	bReturn = CreateGLTexture(
		"../7-1_FinalProjectMilestones/textures/water.jpg",
		"water");


	BindGLTextures();

}
/***********************************************************
 *  CreateMirrored texture
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
bool SceneManager::CreateMirroredTexture(const char* filename, std::string tag)
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
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
 *  SetTwoTextures
 *
 *  This method is used for passing the texture values
 *  into the shader. Requires two textures tags and a mixFactor
 *  that determines the percentage of the second texture
 *  Must be used in place of SetShaderTexture due to edits in 
 *  the fragment shader class expecting two textures.
 ***********************************************************/
void SceneManager::SetTwoTextures(std::string tag1, std::string tag2, float mixFactor)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setFloatValue(g_MixFactor, mixFactor);
		m_pShaderManager->setBoolValue(g_UseTextureName, true);
		m_pShaderManager->setIntValue(g_BlendTexture, true);

		int textureID1 = -1;
		int textureID2 = -1;

		textureID1 = FindTextureSlot(tag1);
		textureID2 = FindTextureSlot(tag2);

		int texture1 = FindTextureID(tag1);
		int texture2 = FindTextureID(tag2);

		glActiveTexture(GL_TEXTURE0 + textureID1);
		glBindTexture(GL_TEXTURE_2D, texture1);

		glActiveTexture(GL_TEXTURE0 + textureID2);
		glBindTexture(GL_TEXTURE_2D, texture2);
		
		
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID1);
		m_pShaderManager->setSampler2DValue(g_Texture2, textureID2);
	}
}
/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{

	// Shiny Plastic Material
	OBJECT_MATERIAL shinyPlasticMaterial;
	shinyPlasticMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f); // Very dark base color
	shinyPlasticMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); // Strong specular reflection
	shinyPlasticMaterial.shininess = 100.0f; // High shininess for glossy plastic
	shinyPlasticMaterial.tag = "shinyPlastic";

	m_objectMaterials.push_back(shinyPlasticMaterial);

	// Flat Plastic Material
	OBJECT_MATERIAL flatPlasticMaterial;
	flatPlasticMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f); // Neutral color
	flatPlasticMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Weak specular reflection
	flatPlasticMaterial.shininess = 5.0f; // Low shininess for flat appearance
	flatPlasticMaterial.tag = "flatPlastic";

	m_objectMaterials.push_back(flatPlasticMaterial);

	// Reflective Glass Material
	OBJECT_MATERIAL reflectiveGlassMaterial;
	reflectiveGlassMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f); // White base color
	reflectiveGlassMaterial.specularColor = glm::vec3(0.95f, 0.95f, 0.95f); // Very strong specular reflection
	reflectiveGlassMaterial.shininess = 120.0f; // Extreme shininess for highly reflective surface
	reflectiveGlassMaterial.tag = "glass";

	m_objectMaterials.push_back(reflectiveGlassMaterial);

	// Bread Material
	OBJECT_MATERIAL breadMaterial;
	breadMaterial.diffuseColor = glm::vec3(0.9f, 0.7f, 0.4f); // Light brown color
	breadMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f); // Minimal specular reflection
	breadMaterial.shininess = 2.0f; // Very low shininess for a soft, matte look
	breadMaterial.tag = "bread";

	m_objectMaterials.push_back(breadMaterial);

	// Vinyl Tablecloth Material
	OBJECT_MATERIAL vinylTableclothMaterial;
	vinylTableclothMaterial.diffuseColor = glm::vec3(0.2f, 0.3f, 0.4f); // Soft muted color
	vinylTableclothMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f); // Medium specular reflection
	vinylTableclothMaterial.shininess = 50.0f; // Moderate shininess for a polished but not overly glossy surface
	vinylTableclothMaterial.tag = "tableCloth";

	m_objectMaterials.push_back(vinylTableclothMaterial);

	// Wallpaper Material
	OBJECT_MATERIAL wallpaperMaterial;
	wallpaperMaterial.diffuseColor = glm::vec3(0.8f, 0.7f, 0.6f); // Light, warm color
	wallpaperMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f); // Very weak specular reflection
	wallpaperMaterial.shininess = 3.0f; // Low shininess for a matte, paper-like texture
	wallpaperMaterial.tag = "wall";

	m_objectMaterials.push_back(wallpaperMaterial);

	// Orange Material
	OBJECT_MATERIAL orangeMaterial;
	orangeMaterial.diffuseColor = glm::vec3(0.9f, 0.5f, 0.1f); // Bright orange base color
	orangeMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Subtle shine to simulate a natural fruit skin
	orangeMaterial.shininess = 10.0f; // Moderate shininess for a slightly textured surface
	orangeMaterial.tag = "orange";

	m_objectMaterials.push_back(orangeMaterial);
}
/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);                      


	// Point light - 0 warm red light
	m_pShaderManager->setVec3Value("pointLights[0].position", -7.0f, 2.0f, 6.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.6f, 0.5f, 0.4f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.8f, 0.4f, 0.1f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.9f, 0.5f, 0.2f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);


	// Point light - 1 cool blue light
	m_pShaderManager->setVec3Value("pointLights[1].position", 8.0f, 2.0f, -6.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.05f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.2f, 0.4f, 1.0f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.3f, 0.6f, 1.0f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Spotlight
	m_pShaderManager->setVec3Value("spotLight.position", -2.0f, 10.0f, 0.0f);
	m_pShaderManager->setVec3Value("spotLight.direction", 0.0f, -1.0f, 0.0f);
	m_pShaderManager->setVec3Value("spotLight.ambient", 1.0f, 0.9f, 0.8f); // Brighter ambient light
	m_pShaderManager->setVec3Value("spotLight.diffuse", 1.5f, 1.3f, 1.2f); // Stronger diffuse light
	m_pShaderManager->setVec3Value("spotLight.specular", 1.5f, 1.3f, 1.2f); // Stronger specular highlights
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f); // Lower to reduce constant attenuation
	m_pShaderManager->setFloatValue("spotLight.linear", 0.1f);   // Reduce to slow down light decay
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.03f); // Lower for longer light range
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(45.0))); // Inner cone angle
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(60.0f))); // Outer cone angle

	m_pShaderManager->setBoolValue("spotLight.bActive", true);

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
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	
	// Call helper functions to establish lights, materials, and bind textures.
	DefineObjectMaterials();
	SetupSceneLights();
	LoadGLTextures();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();

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
	
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

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

	//SetShaderColor(1, 1, 1, 1);
	
	SetTextureUVScale(4.0f, 4.0f);
	SetTwoTextures("cloth", "skull", 0.3f);
	SetShaderMaterial("tableCloth");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/*******************************/
	/* Transformations for PLANE
	*************BACKDROP***********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	
	SetTextureUVScale(3.0f, 3.0f);
	SetTwoTextures("wall", "skull", 0.5f);

	SetShaderMaterial("wall");

	m_basicMeshes->DrawPlaneMesh();

	/*******************************/
	/* Transformations for BOX MESH
	******LOAF OF BREAD************/
	scaleXYZ = glm::vec3(5.0f, 3.0f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-4.0f, 1.0f, 4.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	//SetShaderColor(1, 0, 0, 1);
	
	SetTextureUVScale(1.0f, 1.0f);
	SetTwoTextures("breadSide", "breadSide", 0.0f);

	SetShaderMaterial("bread");

	m_basicMeshes->DrawBoxMesh();
	/*******************************/
	/* Transformations for CYLINDER 
	******LOAF OF BREAD************/

	scaleXYZ = glm::vec3(1.8f, 4.8f, 1.0f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-6.2f, 3.0f, 4.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetTwoTextures("breadTop", "breadTop", 0.0f);
	SetShaderMaterial("bread");

	m_basicMeshes->DrawCylinderMesh();

	/*******************************/
	/* Transformations for CYLINDER
	******BUTTER CONTAINER**********/

	scaleXYZ = glm::vec3(1.5f, 2.0f, 1.5f);
	
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(7.0f, 0.0f, 4.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	SetTextureUVScale(2.0f, 1.0f);
	SetTwoTextures("butter", "butter", 0.0);
	SetShaderMaterial("glass");

	//SetShaderColor(1, 1, 1, 1);

	m_basicMeshes->DrawCylinderMesh();

	/*******************************/
	/* Transformations for CYLINDER
	******BUTTER CONTAINER**********/
	//(LID)
	scaleXYZ = glm::vec3(1.5f, 0.1, 1.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(7.0f, 2.0f, 4.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 0.8, 1);

	SetShaderMaterial("glass");
	
	m_basicMeshes->DrawCylinderMesh();

	/*******************************/
	/* Transformations for TAPEREDCYLINDER
	******BUTTER CONTAINER**********/
	//(bottom)

	scaleXYZ = glm::vec3(1.0f, 0.5f, 1.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(7.0f, 2.0f, 4.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 0.8, 1);

	SetShaderMaterial("glass");


	m_basicMeshes->DrawTaperedCylinderMesh();

	/*******************************/
	/* Transformations for TAPEREDCYLINDER
	******BUTTER CONTAINER**********/
	//(top)

	scaleXYZ = glm::vec3(1.0f, 0.5f, 1.0f);

	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(7.0f, 3.0f, 4.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 0.8, 1);

	SetShaderMaterial("glass");

	m_basicMeshes->DrawTaperedCylinderMesh();

	/*******************************/
	/* Transformations for SPHERE
	***************ORANGE**********/

	scaleXYZ = glm::vec3(1.5f, 1.0f, 1.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(1.0f, 1.0f, 7.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetTextureUVScale(1.0f, 1.0f);
	SetTwoTextures("orange", "cracks", 0.2f);

	SetShaderMaterial("orange");

	m_basicMeshes->DrawSphereMesh();

	/*******************************/
	/* Transformations for CYLINDER
	***************ORANGE**********/

	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.1f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(1.0f, 2.0f, 7.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("bread");

	SetShaderColor(0.5, 0.3, 0.2, 1);

	m_basicMeshes->DrawCylinderMesh();

	/*******************************/
	/* Transformations for CYLINDER
	***********BOTTLE**************/
	//(BASE)

	scaleXYZ = glm::vec3(1.5f, 5.0f, 1.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(4.0f, 0.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("shinyPlastic");

	SetShaderColor(0.8, 0.8, 0.8, 0.6);

	m_basicMeshes->DrawCylinderMesh();

	/*******************************/
	/* Transformations for TAPEREDCYLINDER
	***********BOTTLE**************/

	scaleXYZ = glm::vec3(1.5f, 0.5f, 1.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(4.0f, 5.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.8, 0.8, 0.6);

	SetShaderMaterial("shinyPlastic");

	m_basicMeshes->DrawTaperedCylinderMesh();

	/*******************************/
	/* Transformations for CYLINDER
	***********BOTTLE**************/
	//(LID)

	scaleXYZ = glm::vec3(1.0f, 0.5f, 0.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(4.0f, 6.0f, 0.1f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetTwoTextures("bottleLid", "bottleLid", 0.0f);
	SetShaderMaterial("flatPlastic");

	//SetShaderColor(0, 0, 1, 1);

	m_basicMeshes->DrawCylinderMesh();

	/*******************************/
	/* Transformations for TORUS
	***********BOTTLE**************/

	scaleXYZ = glm::vec3(1.0f, 0.4f, 0.8f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(3.0f, 6.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0.8, 0, 1);

	SetShaderMaterial("flatPlastic");

	m_basicMeshes->DrawTorusMesh();
}
