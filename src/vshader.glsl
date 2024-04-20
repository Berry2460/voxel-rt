#version 430

in vec4 vPosition;
out vec4 vPos;

void main(){
	vPos=vPosition;
	gl_Position=vPosition;
}
