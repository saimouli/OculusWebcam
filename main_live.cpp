/**********************************
** Rendering webcams in to Oculus Rift
   Saimouli Katragadda 2016
**********************************/
 
#include <iostream>
#include <Windows.h>

#include <GL/glew.h>

#include <stddef.h>

#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <opencv2/opencv.hpp>

//#include <SDL2/SDL.h>
//#include <SDL2/SDL_syswm.h>
#include <SDL.h>
#undef main
#include <SDL_syswm.h>

#include <Extras/OVR_Math.h>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

#if OPENGL_GPU_INTEROP

#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

#endif

#include "Shader.hpp"

#define MAX_FPS 120

GLchar* OVR_cam_VS = 
			"#version 330 core\n \
			layout(location=0) in vec3 in_vertex;\n \
			layout(location=1) in vec2 in_texCoord;\n \
			uniform float hit; \n \
			uniform uint isLeft; \n \
			out vec2 b_coordTexture; \n \
			void main()\n \
			{\n \
				if (isLeft == 1U)\n \
				{\n \
					b_coordTexture = in_texCoord;\n \
					gl_Position = vec4(in_vertex.x - hit, in_vertex.y, in_vertex.z,1);\n \
				}\n \
				else \n \
				{\n \
					b_coordTexture = vec2(1.0 - in_texCoord.x, in_texCoord.y);\n \
					gl_Position = vec4(-in_vertex.x + hit, in_vertex.y, in_vertex.z,1);\n \
				}\n \
			}";
#if OPENGL_GPU_INTEROP
GLchar* OVR_cam_FS =
			"#version 330 core\n \
			uniform sampler2D u_texturecam; \n \
			in vec2 b_coordTexture;\n \
			out vec4 out_color; \n \
			void main()\n \
			{\n \
				out_color = vec4(texture(u_texturecam, b_coordTexture).bgr,1); \n \
			}";
#else
GLchar* OVR_cam_FS =
			"#version 330 core\n \
			uniform sampler2D u_texturecam; \n \
			in vec2 b_coordTexture;\n \
			out vec4 out_color; \n \
			void main()\n \
			{\n \
				out_color = vec4(texture(u_texturecam, b_coordTexture).rgb,1); \n \
			}";
#endif

using namespace std;

void cam(float,float);


int main(int argc, char** argv) {
	// Initialize SDL2 context
	SDL_Init(SDL_INIT_VIDEO);
	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result)){
		cout << "OVR_FAILURE" << endl;
		SDL_Quit();
		return -1;
	}

	ovrSession session;
	ovrGraphicsLuid luid;

	//oculus hmd
	result = ovr_Create(&session, &luid); //missinf amp !DEBUG
	if (OVR_FAILURE(result)){
		cout << "Oculus Rift not detected" << endl;
		ovr_Shutdown();
		SDL_Quit();
		return -1;
	}

	//ovrHmdDesc desc = ovr_GetHmdDesc(session);
	//ovrSizei resolution = desc.Resolution;

	int x = SDL_WINDOWPOS_CENTERED, y = SDL_WINDOWPOS_CENTERED;
	int winWidth = 1280;
	int winHeight = 720;
	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
	// Create SDL2 Window
	SDL_Window* window = SDL_CreateWindow("Webcam Oculus App", x, y, winWidth, winHeight, flags);
	// Create OpenGL context
	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	// Initialize GLEW
	glewInit();
	// Turn off vsync to let the compositor do its magic
	SDL_GL_SetSwapInterval(0);

	//initialize webcams
	cv::VideoCapture LeftCam(0), RightCam(1);

	int WidthL = 1220;
	int HeightL = 720;

	LeftCam.set(CV_CAP_PROP_FRAME_WIDTH, WidthL);
	LeftCam.set(CV_CAP_PROP_FRAME_HEIGHT, HeightL);

	RightCam.set(CV_CAP_PROP_FRAME_WIDTH, WidthL);
	RightCam.set(CV_CAP_PROP_FRAME_HEIGHT, HeightL);


	//OpenGL textures

	GLuint camTextureID_L, camTextureID_R;
	// Generate OpenGL texture for left images of the camera
	glGenTextures(1, &camTextureID_L);
	glBindTexture(GL_TEXTURE_2D, camTextureID_L);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WidthL, HeightL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Generate OpenGL texture for right images of the camera
	glGenTextures(1, &camTextureID_R);
	glBindTexture(GL_TEXTURE_2D, camTextureID_R);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WidthL, HeightL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);

	//oculus swap texture
	// Configure Stereo settings.
	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
	// Initialize OpenGL swap textures to render
	ovrTextureSwapChain textureChain = nullptr;

	ovrSizei Tex0Size = ovr_GetFovTextureSize(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0], 1.0f);
	ovrSizei Tex1Size = ovr_GetFovTextureSize(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1], 1.0f);
	ovrSizei bufferSize;
	bufferSize.w = Tex0Size.w + Tex1Size.w;
	bufferSize.h = max(Tex0Size.h, Tex1Size.h);
	// Description of the swap chain
	ovrTextureSwapChainDesc descTextureSwap = {};
	descTextureSwap.Type = ovrTexture_2D;
	descTextureSwap.ArraySize = 1;
	descTextureSwap.Width = bufferSize.w;
	descTextureSwap.Height = bufferSize.h;
	descTextureSwap.MipLevels = 1;
	descTextureSwap.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	descTextureSwap.SampleCount = 1;
	descTextureSwap.StaticImage = ovrFalse;
	// Create the OpenGL texture swap chain
	result = ovr_CreateTextureSwapChainGL(session, &descTextureSwap, &textureChain);
	int length = 0;
	ovr_GetTextureSwapChainLength(session, textureChain, &length);

	if (OVR_SUCCESS(result)){
		// Sample texture access:
		for (int i = 0; i < length; ++i){
			GLuint chainTexId;
			ovr_GetTextureSwapChainBufferGL(session, textureChain, i, &chainTexId);
			glBindTexture(GL_TEXTURE_2D, chainTexId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}
	else{
		cout << "ERROR: failed creating swap texture" << endl;
		ovr_Destroy(session);
		ovr_Shutdown();
		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
		LeftCam.release();
		RightCam.release();
		return -1;
	};

	// Generate frame buffer to render
	GLuint fboID;
	glGenFramebuffers(1, &fboID);
	// Generate depth buffer of the frame buffer
	GLuint depthBuffID;
	glGenTextures(1, &depthBuffID);
	glBindTexture(GL_TEXTURE_2D, depthBuffID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLenum internalFormat = GL_DEPTH_COMPONENT24;
	GLenum type = GL_UNSIGNED_INT;
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, bufferSize.w, bufferSize.h, 0, GL_DEPTH_COMPONENT, type, NULL);

	// Create a mirror texture to display the render result in the SDL2 window
	ovrMirrorTextureDesc descMirrorTexture;
	memset(&descMirrorTexture, 0, sizeof(descMirrorTexture));
	descMirrorTexture.Width = winWidth;
	descMirrorTexture.Height = winHeight;
	descMirrorTexture.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

	ovrMirrorTexture mirrorTexture = nullptr;
	result = ovr_CreateMirrorTextureGL(session, &descMirrorTexture, &mirrorTexture);
	if (!OVR_SUCCESS(result)){
		cout << "ERROR: Failed to create mirror texture" << endl;
	}
	GLuint mirrorTextureId;
	ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &mirrorTextureId);

	GLuint mirrorFBOID;
	glGenFramebuffers(1, &mirrorFBOID);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBOID);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTextureId, 0);
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	// Frame index used by the compositor
	// it needs to be updated each new frame
	long long frameIndex = 0;

	// FloorLevel will give tracking poses where the floor height is 0
	ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);

	// FloorLevel will give tracking poses where the floor height is 0
	ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);

	// Initialize a default Pose
	ovrPosef eyeRenderPose[2];

	// Get the render description of the left and right "eyes" of the Oculus headset
	ovrEyeRenderDesc eyeRenderDesc[2];
	eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);
	// Get the Oculus view scale description
	ovrVector3f hmdToEyeOffset[2];
	double sensorSampleTime;

	Shader shader(OVR_cam_VS, OVR_cam_FS);

	// Compute the cam image field of view with the cam parameters
	float camFovH = 1.f;//atanf(WidthL / (cam->getParameters()->LeftCam.fx *2.f)) * 2.f;//atanf(cam->getImageSize().width / (cam->getParameters()->LeftCam.fx *2.f)) * 2.f;
	// Compute the Horizontal Oculus' field of view with its parameters
	float ovrFovH = (atanf(hmdDesc.DefaultEyeFov[0].LeftTan) + atanf(hmdDesc.DefaultEyeFov[0].RightTan));
	// Compute the useful part of the cam image
	unsigned int usefulWidth = 1220* ovrFovH/ camFovH; //cam->getImageSize().width * ovrFovH / camFovH;
	// Compute the size of the final image displayed in the headset with the cam image's aspect-ratio kept
	unsigned int widthFinal = bufferSize.w / 2;
	float heightGL = 1.f;
	float widthGL = 1.f;
	if (usefulWidth > 0.f){
		unsigned int heightFinal = HeightL * widthFinal / usefulWidth;//cam->getImageSize().height * widthFinal / usefulWidth;
		// Convert this size to OpenGL viewport's frame's coordinates
		heightGL = (heightFinal) / (float)(bufferSize.h);
		widthGL = ((WidthL * (heightFinal / (float)HeightL)) / (float)widthFinal);
	}

	else{
		cout << "WARNING: cam parameters got wrong values."
			"Default vertical and horizontal FOV are used.\n"
			"Check your calibration file or check if your cam is not too close to a surface or an object."
			<< endl;
	}

	// Compute the Vertical Oculus' field of view with its parameters
	float ovrFovV = (atanf(hmdDesc.DefaultEyeFov[0].UpTan) + atanf(hmdDesc.DefaultEyeFov[0].DownTan));

	// Compute the center of the optical lenses of the headset
	float offsetLensCenterX = ((atanf(hmdDesc.DefaultEyeFov[0].LeftTan)) / ovrFovH) * 2.f - 1.f;
	float offsetLensCenterY = ((atanf(hmdDesc.DefaultEyeFov[0].UpTan)) / ovrFovV) * 2.f - 1.f;


	// Create a rectangle with the computed coordinates and push it in GPU memory.
	struct GLScreenCoordinates{
		float left, up, right, down;
	} screenCoord;

	screenCoord.up = heightGL + offsetLensCenterY;
	screenCoord.down = heightGL - offsetLensCenterY;
	screenCoord.right = widthGL + offsetLensCenterX;
	screenCoord.left = widthGL - offsetLensCenterX;

	float rectVertices[12] = { -screenCoord.left, -screenCoord.up, 0,
		screenCoord.right, -screenCoord.up, 0,
		screenCoord.right, screenCoord.down, 0,
		-screenCoord.left, screenCoord.down, 0 };

	GLuint rectVBO[3];
	glGenBuffers(1, &rectVBO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);

	float rectTexCoord[8] = { 0, 1, 1, 1, 1, 0, 0, 0 };
	glGenBuffers(1, &rectVBO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectTexCoord), rectTexCoord, GL_STATIC_DRAW);

	unsigned int rectIndices[6] = { 0, 1, 2, 0, 2, 3 };
	glGenBuffers(1, &rectVBO[2]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectVBO[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIndices), rectIndices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Initialize hit value
	float hit = 0.02f;
	// Initialize a boolean that will be used to stop the applications loop and another one to pause/unpause rendering
	bool end = false;
	bool refresh = true;
	// SDL variable that will be used to store input events
	SDL_Event events;
	// Initialize time variables. They will be used to limit the number of frames rendered per second.
	// Frame counter
	unsigned int riftc = 0, camc = 1;
	// Chronometer
	unsigned int rifttime = 0, camtime = 0, camFPS = 0;
	int time1 = 0, timePerFrame = 0;
	int frameRate = (int)(1000 / MAX_FPS);

	// This boolean is used to test if the application is focused
	bool isVisible = true;

	// Enable the shader
	glUseProgram(shader.getProgramId());
	// Bind the Vertex Buffer Objects of the rectangle that displays cam images
	// vertices
	glEnableVertexAttribArray(Shader::ATTRIB_VERTICES_POS);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO[0]);
	glVertexAttribPointer(Shader::ATTRIB_VERTICES_POS, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectVBO[2]);
	// texture coordinates
	glEnableVertexAttribArray(Shader::ATTRIB_TEXTURE2D_POS);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO[1]);
	glVertexAttribPointer(Shader::ATTRIB_TEXTURE2D_POS, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Main loop
	while (!end){
		// Compute the time used to render the previous frame
		timePerFrame = SDL_GetTicks() - time1;
		// If the previous frame has been rendered too fast
		if (timePerFrame < frameRate){
			// Pause the loop to have a max FPS equal to MAX_FPS
			SDL_Delay(frameRate - timePerFrame);
			timePerFrame = frameRate;
		}
		// Increment the cam chronometer
		camtime += timePerFrame;
		// If cam chronometer reached 1 second
		if (camtime > 1000){
			camFPS = camc;
			camc = 0;
			camtime = 0;
		}
		// Increment the Rift chronometer and the Rift frame counter
		rifttime += timePerFrame;
		riftc++;
		// If Rift chronometer reached 200 milliseconds
		if (rifttime > 200){
			// Display FPS
			cout << "\rRIFT FPS: " << 1000 / (rifttime / riftc) << " | cam FPS: " << camFPS << endl;
			// Reset Rift chronometer
			rifttime = 0;
			// Reset Rift frame counter
			riftc = 0;
		}
		// Start frame chronometer
		time1 = SDL_GetTicks();

		// While there is an event catched and not tested
		while (SDL_PollEvent(&events)){
			// If a key is released
			if (events.type == SDL_KEYUP){
				// If Q quit the application
				if (events.key.keysym.scancode == SDL_SCANCODE_Q)
					end = true;
				// If R reset the hit value
				else if (events.key.keysym.scancode == SDL_SCANCODE_R)
					hit = 0.0f;
				// If C pause/unpause rendering
				else if (events.key.keysym.scancode == SDL_SCANCODE_C)
					refresh = !refresh;
			}
			// If the mouse wheel is used
			if (events.type == SDL_MOUSEWHEEL){
				// Increase or decrease hit value
				float s;
				events.wheel.y > 0 ? s = 1.0f : s = -1.0f;
				hit += 0.005f * s;
			}
		}

		// Get texture swap index where we must draw our frame
		GLuint curTexId;
		int curIndex;
		ovr_GetTextureSwapChainCurrentIndex(session, textureChain, &curIndex);
		ovr_GetTextureSwapChainBufferGL(session, textureChain, curIndex, &curTexId);

		// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
		eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
		eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);
		hmdToEyeOffset[0] = eyeRenderDesc[0].HmdToEyeOffset;
		hmdToEyeOffset[1] = eyeRenderDesc[1].HmdToEyeOffset;
		// Get eye poses, feeding in correct IPD offset
		ovr_GetEyePoses(session, frameIndex, ovrTrue, hmdToEyeOffset, eyeRenderPose, &sensorSampleTime);

		// If the application is focused
		if (isVisible){
			// If successful grab a new cam image
			cv::Mat frameL,frameR;
			LeftCam >> frameL; RightCam>> frameR;
			if (!frameL.empty() && !frameR.empty()){
				// Update the cam frame counter
				cout << "frameR not empty "<< endl;
				camc++;
				if (refresh){

					// Bind the frame buffer
					glBindFramebuffer(GL_FRAMEBUFFER, fboID);
					// Set its color layer 0 as the current swap texture
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
					// Set its depth layer as our depth buffer
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffID, 0);
					// Clear the frame buffer
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					glClearColor(0, 0, 0, 1);

					// Render for each Oculus eye the equivalent cam image
					for (int eye = 0; eye < 2; eye++){
						// Set the left or right vertical half of the buffer as the viewport
						glViewport(eye == ovrEye_Left ? 0 : bufferSize.w / 2, 0, bufferSize.w / 2, bufferSize.h);
						// Bind the left or right cam image
						glBindTexture(GL_TEXTURE_2D, eye == ovrEye_Left ? camTextureID_L : camTextureID_R);				
						//std::cout << "camHeight= " << frameL.col << std::endl;
						glTexImage2D(GL_TEXTURE_2D, 0, GL_BGR, WidthL, HeightL, 0, GL_BGR, GL_UNSIGNED_BYTE,(eye == ovrEye_Left ? frameL:frameR).data);//(eye == ovrEye_Left ? frameR:frameL).data);

						// Bind the hit value
						glUniform1f(glGetUniformLocation(shader.getProgramId(), "hit"), eye == ovrEye_Left ? hit : -hit);
						// Bind the isLeft value
						glUniform1ui(glGetUniformLocation(shader.getProgramId(), "isLeft"), eye == ovrEye_Left ? 1U : 0U);
						// Draw the cam image
						glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
					}

					// Avoids an error when calling SetAndClearRenderSurface during next iteration.
					// Without this, during the next while loop iteration SetAndClearRenderSurface
					// would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
					// associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
					glBindFramebuffer(GL_FRAMEBUFFER, fboID);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
					// Commit changes to the textures so they get picked up frame
					ovr_CommitTextureSwapChain(session, textureChain);
				}

				// Do not forget to increment the frameIndex!
				frameIndex++;
			}
		}
		/*
		Note: Even if we don't ask to refresh the framebuffer or if the Camera::grab()
		doesn't catch a new frame, we have to submit an image to the Rift; it
		needs 75Hz refresh. Else there will be jumbs, black frames and/or glitches
		in the headset.
		*/

		ovrLayerEyeFov ld;
		ld.Header.Type = ovrLayerType_EyeFov;
		// Tell to the Oculus compositor that our texture origin is at the bottom left
		ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL | Disable head tracking
		// Set the Oculus layer eye field of view for each view
		for (int eye = 0; eye < 2; ++eye){
			// Set the color texture as the current swap texture
			ld.ColorTexture[eye] = textureChain;
			// Set the viewport as the right or left vertical half part of the color texture
			ld.Viewport[eye] = OVR::Recti(eye == ovrEye_Left ? 0 : bufferSize.w / 2, 0, bufferSize.w / 2, bufferSize.h);
			// Set the field of view
			ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
			// Set the pose matrix
			ld.RenderPose[eye] = eyeRenderPose[eye];
		}

		ld.SensorSampleTime = sensorSampleTime;

		ovrLayerHeader* layers = &ld.Header;
		// Submit the frame to the Oculus compositor
		// which will display the frame in the Oculus headset
		cout << "frame Index:"<<frameIndex << endl;
		result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);

		if (!OVR_SUCCESS(result)){
			cout << "ERROR: failed to submit frame" << endl;
			glDeleteBuffers(3, rectVBO);
			ovr_DestroyTextureSwapChain(session, textureChain);
			ovr_DestroyMirrorTexture(session, mirrorTexture);
			ovr_Destroy(session);
			ovr_Shutdown();
			SDL_GL_DeleteContext(glContext);
			SDL_DestroyWindow(window);
			SDL_Quit();
			LeftCam.release();
			RightCam.release();
			return -1;
		}

		if (result == ovrSuccess && !isVisible){
			cout << "The application is now shown in the headset." << endl;
		}
		isVisible = (result == ovrSuccess);

		// This is not really needed for this application but it may be usefull for an more advanced application
		ovrSessionStatus sessionStatus;
		ovr_GetSessionStatus(session, &sessionStatus);

		if (sessionStatus.ShouldRecenter){
			cout << "Recenter Tracking asked by Session" << endl;
			ovr_RecenterTrackingOrigin(session);
		}

		// Copy the frame to the mirror buffer
		// which will be drawn in the SDL2 image
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBOID);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		GLint w = winWidth;
		GLint h = winHeight;
		glBlitFramebuffer(0, h, w, 0,
			0, 0, w, h,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		// Swap the SDL2 window
		SDL_GL_SwapWindow(window);
	}

	// Disable all OpenGL buffer
	glDisableVertexAttribArray(Shader::ATTRIB_TEXTURE2D_POS);
	glDisableVertexAttribArray(Shader::ATTRIB_VERTICES_POS);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
	glBindVertexArray(0);
	// Delete the Vertex Buffer Objects of the rectangle
	glDeleteBuffers(3, rectVBO);
	// Delete SDL, OpenGL, Oculus and cam context
	ovr_DestroyTextureSwapChain(session, textureChain);
	ovr_DestroyMirrorTexture(session, mirrorTexture);
	ovr_Destroy(session);
	ovr_Shutdown();
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	LeftCam.release();
	RightCam.release();
	// quit
	system("pause");
	return 0;
}



