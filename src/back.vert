
#version 330 core

in vec3 vertexPosition;
in vec2 vertexTexCoords; // Solo necesitamos la posición y las coordenadas UV

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec2 fragTexCoords;

void main() {
	// Se aplican las transformaciones model, view y projection para obtener la posición en pantalla[cite: 1].
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);
	
	// Se pasan las coordenadas de textura intactas al fragment shader para que las convierta en color
	fragTexCoords = vertexTexCoords;
}
