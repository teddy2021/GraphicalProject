#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <unordered_map>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#ifndef HEADER_SIMPLE_OPENGL_IMAGE_LIBRARY
#include <SOIL2.h>
#endif

#ifndef DISPLAY
#define DISPLAY
#include "display.hpp"
#endif

#ifndef CON
#define CON
#include "convenience.hpp"
#endif

#ifndef OGLHPP
#define OGLHPP
#include "ogl.hpp"
#endif

using namespace std;
using namespace glm;

float width = 1920, height = 1080;

Display_Object* objects;
int n_objects = 0;
int num_shaders;
GLuint* shaders;
vector<GLuint*> shader_uniforms;
vector<GLenum*> uniform_types;
vector<GLchar**> uniform_names;
unordered_map<GLchar*, void*> uniform_values;

GLuint VBO;

GLuint vert_b, uv_b, norm_b, idx_b;

GLuint Frame_buffer;
GLuint depthprogram;
GLuint depthUniform;
GLuint depthTexture;

vec3 lightInvDir;

mat4 depthProjection, depthView, depthModel, depthMVP;
mat4 depthBiasMVP;

GLFWwindow * setup_display(){
	GLFWwindow * window = setup();
	num_shaders = 0;
	
	glGenVertexArrays(1, &VBO);
	glBindVertexArray(VBO);

	// Depth shader relevant details
	// The depth shader is used to create realtime shadows
	depthprogram = addShader("Depth");

	depthUniform = shader_uniforms.at(0)[1];
	
	lightInvDir = vec3(0.5, 2, 2);

	Frame_buffer = 0;
	glGenFramebuffers(1, &Frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, Frame_buffer);

	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 1024, 1024, 0,
			GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, 
			GL_COMPARE_R_TO_TEXTURE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);
	glDrawBuffer(GL_NONE);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		fprintf(stderr, "Failed to complete framebuffer status while making"
				" depth buffer.\n");
	}

	return window;
}

void computeDepthMatrices(){
	depthProjection = ortho<float>(-10,10, -10,10, -10,20);
	depthView = lookAt(lightInvDir, vec3(0,0,0), vec3(0,1,0));
	depthModel = mat4(1);
	depthMVP = depthProjection * depthView * depthModel;
	depthBiasMVP =  mat4(
			vec4(0.5,0,0,0),
			vec4(0,0.5,0,0),
			vec4(0,0,0.5,0),
			vec4(0.5,0.5,0.5,1)
			)* depthMVP;

}

void handleShadows(){
	computeDepthMatrices();
	glUniformMatrix4fv(depthUniform, 1, GL_FALSE, &depthMVP[0][0]);
	
	glUseProgram(depthprogram);
	glBindFramebuffer(GL_FRAMEBUFFER, Frame_buffer);
	glViewport(0,0,1024,1024);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for(int i = 0; i < n_objects; i += 1){
		Display_Object obj = objects[i];
		GLuint* buffers = obj.GetBuffers();
		Mesh mesh = obj.GetMesh();

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, buffers[0]); // pointer to the 0th element 
		// in the buffer array, so handing it directly is the same as buffers[0]
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[4]);

		glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_SHORT, 0);
		glDisableVertexAttribArray(0);

	}

	glBindBuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);

}

void setUniform(GLuint uni, GLenum typ, GLchar* nm){
	void *value = (void*)uniform_values.at(nm);
	switch (typ){

		// floats
		case GL_FLOAT:
			glUniform1f(uni, *(float*)value);
			break;
		case GL_FLOAT_VEC2 :
			glUniform2fv(uni, 1, (float*)value);
			break;
		case GL_FLOAT_VEC3:
			glUniform3fv(uni, 1, (float*)value);
				break;
		case GL_FLOAT_VEC4:
			glUniform4fv(uni, 1, (float*)value);
			break;

		// ints
		case GL_INT:
			glUniform1i(uni, *(int*)value);
			break;
		case GL_INT_VEC2:
			glUniform2iv(uni, 1, (int*)value);
			break;
		case GL_INT_VEC3:
			glUniform3iv(uni, 1, (int*)value);
			break;
		case GL_INT_VEC4:
			glUniform4iv(uni, 1, (int*)value);
			break;

		// uints
		case GL_UNSIGNED_INT:
			glUniform1ui(uni, *(unsigned int*)value);
			break;
		case GL_UNSIGNED_INT_VEC2:
			glUniform2uiv(uni, 1, (unsigned int*)value);
			break;
		case GL_UNSIGNED_INT_VEC3:
			glUniform3uiv(uni, 1, (unsigned int*)value);
			break;
		case GL_UNSIGNED_INT_VEC4:
			glUniform4uiv(uni, 1, (unsigned int*)value);
			break;

		// bools
		case GL_BOOL:
			glUniform1i(uni, *(int*)value);
			break;
		case GL_BOOL_VEC2:
			glUniform2iv(uni, 1, (int*)value);
			break;
		case GL_BOOL_VEC3:
			glUniform3iv(uni, 1, (int*)value);
			break;
		case GL_BOOL_VEC4:
			glUniform4iv(uni, 1, (int*)value);
			break;

		//float square mats
		case GL_FLOAT_MAT2:
			glUniformMatrix2fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_FLOAT_MAT3: 
			glUniformMatrix3fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_FLOAT_MAT4:
			glUniformMatrix4fv(uni, 1, GL_FALSE, (float*)value);
			break;

		//float mats
		case GL_FLOAT_MAT2x3:
			glUniformMatrix2x3fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_FLOAT_MAT2x4:
			glUniformMatrix2x4fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_FLOAT_MAT3x2:
			glUniformMatrix3x2fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_FLOAT_MAT3x4:
			glUniformMatrix3x4fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_FLOAT_MAT4x2:
			glUniformMatrix4x2fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_FLOAT_MAT4x3:
			glUniformMatrix4x3fv(uni, 1, GL_FALSE, (float*)value);
			break;

		//double square mats
		case GL_DOUBLE_MAT2:
			glUniformMatrix2fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_DOUBLE_MAT3: 
			glUniformMatrix3fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_DOUBLE_MAT4:
			glUniformMatrix4fv(uni, 1, GL_FALSE, (float*)value);
			break;

		//double mats
		case GL_DOUBLE_MAT2x3:
			glUniformMatrix2x3fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_DOUBLE_MAT2x4:
			glUniformMatrix2x4fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_DOUBLE_MAT3x2:
			glUniformMatrix3x2fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_DOUBLE_MAT3x4:
			glUniformMatrix3x4fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_DOUBLE_MAT4x2:
			glUniformMatrix4x2fv(uni, 1, GL_FALSE, (float*)value);
			break;
		case GL_DOUBLE_MAT4x3:
			glUniformMatrix4x3fv(uni, 1, GL_FALSE, (float*)value);
			break;

		//samplers
		case GL_SAMPLER_1D:
			glUniform1i(uni, *(int*)value);
			break;
		case GL_SAMPLER_2D:
			glUniform1i(uni, *(int*)value);
			break;
		case GL_SAMPLER_3D:
			glUniform1i(uni, *(int*)value);
			break;
		case GL_SAMPLER_CUBE:
			glUniform1i(uni, *(int*)value);
			break;


		//shadow samplers
		case GL_SAMPLER_1D_SHADOW:
			glUniform1i(uni, *(int*)value);
			break;
		case GL_SAMPLER_2D_SHADOW:
			glUniform1i(uni, *(int*)value);
			break;

			
		default:
			fprintf(stderr,
			"Unknown or unhandled type for uniform %d with name %s\n"
			"Type was %u\n", uni, nm, typ);
			return;
	}
}

void setUniforms(int program){
	GLuint * uniformset = shader_uniforms.at(program);
	GLenum * typeset = uniform_types.at(program);
	GLchar** nameset = uniform_names.at(program);

	for(int i = 1; i < uniformset[0]; i += 1){
		GLuint uniform = uniformset[i];
		GLenum type = typeset[i-1];
		GLchar* name = nameset[i-1];

	}
}

void display(){
	handleShadows();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLuint* buffers;
	Mesh mesh;
	GLuint texture;

	for(int i = 0; i < n_objects; i += 1){
		Display_Object obj = objects[i];
		buffers = obj.GetBuffers();
		mesh = obj.GetMesh();
		glUseProgram(shaders[obj.getShaderIdx()]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, obj.getTexture());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthTexture);

	
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, *buffers);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, *(buffers + 1));
	
		
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, *(buffers + 2));

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *(buffers + 3));
		

		setUniforms(obj.getShaderIdx());


		glDrawElements(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size(), 
				GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}
}

void AssociateShader(int shader_idx, int object_idx){

	objects[object_idx].setShaderIdx(shader_idx);
}

void AssociateUniform(GLchar *name, void *value){
	std::pair<GLchar *, void *> element(name, value);
	uniform_values.insert(element);
}

void GetUniformSet(int program, 
		GLuint* &uniforms,
		GLenum* &typeset,
		GLchar** &nameset){
	GLint count, size;
	GLenum type;
	GLuint i;

	const GLsizei bufsize = 16;
	GLchar name[bufsize];
	GLsizei length;
	glGetProgramiv(shaders[program], GL_ACTIVE_ATTRIBUTES, &count);

 	*uniforms = count;

	uniforms = (GLuint*)realloc(uniforms, count + 1);
	typeset = (GLenum*)realloc(typeset, count);
	nameset = (GLchar**)realloc(nameset, count);
	

	vector<GLuint> unifs;
	vector<GLenum> types;
	printf("[-1.%d] getting uniforms for shader %d\n", program, program);
	for(i = 0; i < count; i += 1){

		printf("[%d.0] getting shader variable for shader %d and index %d\n", i, program,  i);
		glGetActiveUniform(shaders[program], i, bufsize, 
				&length, &size, &type, name);

		printf("[%d.1] storing uniform index\n", i);
		memcpy((uniforms + i), &i, sizeof(GLuint));
		printf("[%d.2] copying type (%u) out\n", i, type);
		memcpy(typeset, &type, sizeof(type));
		printf("[%d.3] allocating space of size %d for char\n", i , length);
		nameset[i] = (GLchar*)malloc(sizeof(GLchar) * length);
		printf("[%d.4] copying %s out to char array\n", i, name);
		memcpy(&nameset[i], name, length);
	}
}

int addShader(string name){
	if(num_shaders == 0){
		shaders = (GLuint *)malloc(sizeof(GLuint));
	}
	else{
		shaders = (GLuint*)realloc(shaders,sizeof(GLuint) * num_shaders + 1);
	}
	num_shaders += 1;
	string vs, fs;
	vs = "resources/shaders/" + name + ".vs";
	fs = "resources/shaders/" + name + ".fs";
	const char* vss = vs.c_str();
	const char* fss = fs.c_str();
	if((shaders[num_shaders - 1] = LoadShaders(vss, fss)) == -1){
		fprintf(stderr, "Failed to load shader program for shader pair %s\n", 
				name.c_str());
		return -1;
	}

	GLuint *program_uniforms = (GLuint*) malloc(sizeof(GLuint));
	GLenum * types = (GLenum*)  malloc(0);
	GLchar** names = (GLchar**)  malloc(0);
	
	GetUniformSet(num_shaders - 1, program_uniforms, types, names);
	
	shader_uniforms.push_back(program_uniforms);
	uniform_types.push_back(types);
	uniform_names.push_back(names);

	return num_shaders-1;
}

int add_display_object(Display_Object obj){
	if(n_objects == 0){
		objects = (Display_Object*)malloc(sizeof(Display_Object));
	}
	else{
		objects = (Display_Object*)realloc(objects, n_objects);
	}
	n_objects += 1;
	memcpy(&objects[n_objects-1], &obj, sizeof(obj));
	return n_objects-1;
}

int add_object_path(const char* obj_path){
	vector<Model> models;
	char location[1024];
	sprintf(location, "resources/%s/%s.models", 
			obj_path, obj_path);
	if(loadModels(location, models) != true){
		fprintf(stderr, "Failed to load from %s models file\n", obj_path);
		return - 1;
	}	
	if(n_objects == 0){
		objects = (Display_Object*)malloc(sizeof(Display_Object) * models.size());
	}
	else{
		objects = (Display_Object*)realloc(objects, n_objects + models.size());
	}	
	n_objects += models.size();
	
	for(Model m : models){
		char object_place[1024];
		sprintf(object_place, "resources/%s/%s", obj_path, m.objFilename.c_str());

		char object_tex_place[1024];
		sprintf(object_tex_place, "resources/%s/%s", obj_path, m.textureFilename.c_str());
		Display_Object obj(object_place, object_tex_place);

		vec3 scaler = vec3(m.sx, m.sy, m.sz);
		vec3 translation = vec3(m.tx, m.ty, m.tz);
		vec3 rotation_axis = vec3(m.rx, m.ry, m.rz);
		
		obj.set_scale(scaler);
		obj.set_translation(translation);
		obj.set_rotation(m.ra, rotation_axis);

		memcpy(&objects[n_objects - 1], &obj, sizeof(obj));
	}

	return n_objects -1;

}

int add_object_buffer(Mesh mesh, GLuint texture){
	if(n_objects == 0){
		objects = (Display_Object*)malloc(sizeof(Display_Object));
	}
	else{
		objects = (Display_Object*)realloc(objects, n_objects);
	}
	n_objects += 1;
	Display_Object obj(mesh, texture);
	memcpy(&objects[n_objects - 1], &obj, sizeof(obj));
	return n_objects - 1;
}


