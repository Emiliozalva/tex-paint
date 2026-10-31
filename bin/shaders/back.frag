#version 330 core 

in vec2 fragTexCoords;

out vec4 fragColor; 

void main(){
	fragColor = vec4(fragTexCoords[0],fragTexCoords[1],0.f,1.f); 
}
