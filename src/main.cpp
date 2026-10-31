#include <algorithm>
#include <stdexcept>
#include <vector>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Model.hpp"
#include "Window.hpp"
#include "Callbacks.hpp"
#include "Debug.hpp"
#include "Shaders.hpp"

#define VERSION 20260831
#include <iomanip>
#include <glm/ext/vector_float3.hpp>
using namespace std;

Window main_window; // ventana principal: muestra el modelo en 3d sobre el que se pinta
Window aux_window; // ventana auxiliar que muestra la textura
void drawMain(); // dibuja el modelo "normalmente" para la ventana principal
void drawBack(); // dibuja el modelo con un shader alternativo para convertir coords de la ventana a coords de textura
void drawAux(); // dibuja la textura en la ventana auxiliar
void drawImGui(Window &window); // settings sub-window

float radius = .5; // radio del "pincel" con el que pintamos en la textura
glm::vec4 color = { 0.f, 0.f, 0.f, 1.f }; // color actual con el que se pinta en la textura

Texture texture; // textura (compartida por ambas ventanas)
Image image; // imagen (para la textura, Image está en RAM, Texture la envía a GPU)

Model model_chookity; // el objeto a pintar, para renderizar en la ventan principal
Model model_aux; // un quad para cubrir la ventana auxiliar y mostrar la textura

Shader shader_main; // shader para el objeto principal (drawMain)
Shader shader_aux; // shader para la ventana auxiliar (drawTexture)
Shader shader_back; 
// callbacks del mouse y auxiliares para los callbacks
enum class MouseAction { None, ManipulateView, Draw };
MouseAction mouse_action = MouseAction::None; // qué hacer en el callback del motion si el botón del mouse está apretado
void mainMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
void mainMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void auxMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
void auxMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

int main() {
	
	// main window (3D view)
	
	main_window = Window(800, 600, "Main View");
	glfwSetCursorPosCallback(main_window, mainMouseMoveCallback);
	glfwSetMouseButtonCallback(main_window, mainMouseButtonCallback);
	main_window.getCamera().model_angle = 2.5;
	
	glClearColor(1.f,1.f,1.f,1.f);
	shader_main = Shader("shaders/main");
	shader_back = Shader("shaders/back");
	
	image = Image("models/chookity.png",true);
	texture = Texture(image);
	
	model_chookity = Model::loadSingle("models/chookity", Model::fNoTextures);
	
	// aux window (texture image)
	Window aux_window(512,512, "Texture", Window::fDefaults, main_window);
	glfwSetCursorPosCallback(aux_window, auxMouseMoveCallback);
	glfwSetMouseButtonCallback(aux_window, auxMouseButtonCallback);
	
	model_aux = Model::loadSingle("models/texquad", Model::fNoTextures);
	shader_aux = Shader("shaders/quad");
	
	// main loop
	do {
		glfwMakeContextCurrent(main_window);
		drawMain();
		drawImGui(main_window);
		glFinish();
		glfwSwapBuffers(main_window);
		
		glfwMakeContextCurrent(aux_window);
		drawAux();
		drawImGui(aux_window);
		glFinish();
		glfwSwapBuffers(aux_window);
		
		glfwPollEvents();
		
	} while( (not glfwWindowShouldClose(main_window)) and (not glfwWindowShouldClose(aux_window)) );
	main_window.disableImGui();
	aux_window.disableImGui();
}


// ===== pasos del renderizado =====

void drawMain() {
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	texture.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	shader_main.use();
	setMatrixes(main_window, shader_main);
	shader_main.setLight(glm::vec4{-1.f,1.f,4.f,1.f}, glm::vec3{1.f,1.f,1.f}, 0.35f);
	shader_main.setMaterial(model_chookity.material);
	shader_main.setBuffers(model_chookity.buffers);
	model_chookity.buffers.draw();
}
void drawBack() {
	glfwMakeContextCurrent(main_window);
	glDisable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST); 
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	shader_back.use();
	setMatrixes(main_window, shader_back); 
	shader_back.setBuffers(model_chookity.buffers); 
	model_chookity.buffers.draw(); 

	glEnable(GL_MULTISAMPLE);
	glFlush();
	glFinish();
	
	glClearColor(1.f, 1.f, 1.f, 1.f);
	
	/// @ToDo: Parte 2: renderizar el modelo en 3d con un nuevo shader de forma 
	///                 que queden las coordenadas de textura de cada fragmento
	///                 en el back-buffer de color
}


void drawAux() {
	glDisable(GL_DEPTH_TEST);
	texture.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	shader_aux.use();
	shader_aux.setMatrixes(glm::mat4{1.f}, glm::mat4{1.f}, glm::mat4{1.f});
	shader_aux.setBuffers(model_aux.buffers);
	model_aux.buffers.draw();
}



void drawImGui(Window &window) {
	if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) return;
	// settings sub-window
	window.ImGuiDialog("Settings",[&](){
		ImGui::SliderFloat("Radius",&radius,.5,50);
		ImGui::ColorEdit4("Color",&(color[0]),0);
		
		static std::vector<std::pair<const char *, ImVec4>> pallete = { // colores predefindos
			{"white" , {1.f,1.f,1.f,1.f}},
		{"pink"  , {0.749f,0.49f,0.498f,1.f}},
			{"yellow", {0.965f,0.729f,0.106f,1.f}},
		{"black" , {0.f,0.f,0.f,1.f}} };
		
		ImGui::Text("Pallete:");
		for (auto &p : pallete) {
			ImGui::SameLine();
			if (ImGui::ColorButton(p.first, p.second))
				color[0] = p.second.x, color[1] = p.second.y, color[2] = p.second.z;
		}
		
		if (ImGui::Button("Reload Image")) {
			image = Image("models/chookity.png",true);
			texture.update(image);
		}
	});
}


void pinta(double s, double t, float esc=1.0) {
	int ancho = image.GetWidth();
	int alto = image.GetHeight();
	float radius1 = esc * radius;
	
	// Hacemos que el borde de antialiasing sea proporcional al radio (ej. 25% extra del radio, mínimo 1.5 px)
	// Así cuando el radio es grande, el difuminado es bien notorio
	float ancho_aa = std::max(1.5f, radius1 * 0.25f);
	float radio_total = radius1 + ancho_aa;
	
	int limite = static_cast<int>(std::ceil(radio_total));
	
	for (int i = -limite; i <= limite; i++) {
		for (int j = -limite; j <= limite; j++) {
			double dist = std::sqrt(i*i + j*j);
			
			if (dist <= radio_total) {
				int coordx = static_cast<int>(std::round(s)) + i;
				int coordy = static_cast<int>(std::round(t)) + j;
				
				if (coordx >= 0 && coordx < ancho && coordy >= 0 && coordy < alto) {
					
					// 1.0 adentro del radio, y cae de 1.0 a 0.0 entre radius1 y radio_total
					float factor_borde = 1.0f;
					if (dist > radius1) {
						factor_borde = static_cast<float>((radio_total - dist) / ancho_aa);
						factor_borde = std::max(0.0f, std::min(factor_borde, 1.0f));
					}
					
					float alpha_efectivo = color[3] * factor_borde;
					
					glm::vec4 color_fondo = image.GetRGBA(coordy, coordx);
					glm::vec4 color_nuevo;
					
					color_nuevo[0] = (alpha_efectivo * color[0]) + ((1.f - alpha_efectivo) * color_fondo[0]);
					color_nuevo[1] = (alpha_efectivo * color[1]) + ((1.f - alpha_efectivo) * color_fondo[1]);
					color_nuevo[2] = (alpha_efectivo * color[2]) + ((1.f - alpha_efectivo) * color_fondo[2]);
					color_nuevo[3] = alpha_efectivo + ((1.f - alpha_efectivo) * color_fondo[3]);
					
					image.SetRGBA(coordy, coordx, color_nuevo);
				}
			}
		}
	}
}
	// ===== callbacks de la ventana auxiliar (textura) =====
	double ult_s,ult_t;
	bool hay_punto_previo = false;
	
	void auxMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		if (ImGui::GetIO().WantCaptureMouse) return;
		if (action == GLFW_PRESS) {
			mouse_action = MouseAction::Draw;
			
			double xpos, ypos; 
			glfwGetCursorPos(window, &xpos, &ypos); 
			BufferSize bs = getBufferSize(window); 
			
			double s = (xpos * image.GetWidth()) / bs.width;
			double t = ((bs.height - ypos) * image.GetHeight()) / bs.height; 
			ult_s = s;
			ult_t = t; 
			hay_punto_previo = true;
			
			pinta(s, t);
			texture.update(image);
		} else {
			mouse_action = MouseAction::None;
			hay_punto_previo = false;
		}
	}
	
	void auxMouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
		if (mouse_action != MouseAction::Draw) return;
		
		int ancho = image.GetWidth();
		int alto = image.GetHeight(); 
		BufferSize bs = getBufferSize(window); 
		
		double s = (xpos * ancho) / bs.width;
		double t = ((bs.height - ypos) * alto) / bs.height;
		
		double dx = s - ult_s;
		double dy = t - ult_t;
		double distancia = std::sqrt(dx * dx + dy * dy);
		
		double espaciado = std::max(1.0, radius * 0.4); 
		if (distancia < espaciado) return;
		
		int relleno = static_cast<int>(distancia / espaciado); 
		
		for (int i = 1; i <= relleno; ++i) {
			double fraccion = static_cast<double>(i) / relleno;
			double s_paso = ult_s + dx * fraccion;
			double t_paso = ult_t + dy * fraccion;
			pinta(s_paso, t_paso);
		}
		
		// Actualizamos exactamente hasta donde llegó el último círculo pintado
		ult_s = s;
		ult_t = t;
		texture.update(image);
	}
	
	
	// ===== callbacks de la ventana principal (vista 3D) =====
	
	void mainMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		if (ImGui::GetIO().WantCaptureMouse) return;
		if (action == GLFW_PRESS) {
			if (mods != 0 or button == GLFW_MOUSE_BUTTON_RIGHT) {
				mouse_action = MouseAction::ManipulateView;
				common_callbacks::mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, action, mods);
				return;
			}
			
			mouse_action = MouseAction::Draw;
			
			double xpos, ypos; 
			glfwGetCursorPos(window, &xpos, &ypos);
			BufferSize bs = getBufferSize(window); 
			drawBack(); 
			
			float pixel[4]; 
			glReadPixels(static_cast<int>(xpos), bs.height - static_cast<int>(ypos) - 1, 1, 1, GL_RGBA, GL_FLOAT, pixel);
			
			if (pixel[3] > 0.0f) {
				double s = pixel[0] * image.GetWidth();
				double t = pixel[1] * image.GetHeight();
				ult_s = s;
				ult_t = t; 
				hay_punto_previo = true;
				
				pinta(s, t);
				texture.update(image);
			} else {
				// Si hizo clic en el fondo blanco (fuera del modelo), aún no hay punto inicial válido
				hay_punto_previo = false;
			}
			
		} else {
			if (mouse_action == MouseAction::ManipulateView)
				common_callbacks::mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, action, mods);
			mouse_action = MouseAction::None;
			hay_punto_previo = false;
		}
	}
	
	void mainMouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
		if (mouse_action != MouseAction::Draw) {
			if (mouse_action == MouseAction::ManipulateView)
				common_callbacks::mouseMoveCallback(window, xpos, ypos);
			return; 
		}
		
		BufferSize bs = getBufferSize(window); 
		drawBack(); 
		float pixel[4];
		glReadPixels(static_cast<int>(xpos), bs.height - static_cast<int>(ypos) - 1, 1, 1, GL_RGBA, GL_FLOAT, pixel);
		
		if (pixel[3] > 0.0f) {
			double s = pixel[0] * image.GetWidth();
			double t = pixel[1] * image.GetHeight();
			
			// Si veníamos desde afuera del modelo, empezamos un trazo nuevo sin unir con el punto viejo
			if (!hay_punto_previo) {
				ult_s = s;
				ult_t = t;
				hay_punto_previo = true;
				pinta(s, t);
				texture.update(image);
				return;
			}
			
			double dx = s - ult_s;
			double dy = t - ult_t;
			double distancia = std::sqrt(dx * dx + dy * dy);
			
			// Si la distancia salta de golpe (ej. más de 100 px), cruzamos una costura UV del modelo 3D:
			// pintamos en el nuevo lugar pero no trazamos una raya cruzando toda la textura
			if (distancia > 100.0) {
				ult_s = s;
				ult_t = t;
				pinta(s, t);
				texture.update(image);
				return;
			}
			
			// Mismo control de espaciado que en 2D para evitar repintar sobre el mismo píxel
			double espaciado = std::max(1.0, radius * 0.4);
			if (distancia < espaciado) return;
			
			int relleno = static_cast<int>(distancia / espaciado); 
			
			for (int i = 1; i <= relleno; ++i) {
				double fraccion = static_cast<double>(i) / relleno;
				double s_paso = ult_s + dx * fraccion;
				double t_paso = ult_t + dy * fraccion;
				pinta(s_paso, t_paso);
			}
			
			ult_s = s;
			ult_t = t;
			texture.update(image);
		} else {
			// El mouse se salió del modelo 3D hacia el fondo: cortamos el trazo continuo
			hay_punto_previo = false;
		}
	}

	
