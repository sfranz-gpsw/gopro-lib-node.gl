#version 450
out gl_PerVertex {
    vec4 gl_Position;   
};

void setNglPos(vec4 pos) {
	gl_Position = pos;
}
