uniform sampler2D myTextureSampler;
uniform int detailMode;
uniform float rOrient;
uniform float rDepth;
uniform float zminDepth;

varying vec3 N;
varying vec3 vPosition;
varying vec3 L;
varying vec3 v;
 
void main(){ 
	vec2 UV;
	UV.x = max(0.0, dot(N,L));
	
	// Orientation-based attribute mapping
	if (detailMode == 0) {
		UV.y = pow(abs(dot(N, v)), rOrient);
	}
	
	// Depth-based attribute mapping
	else {
		float z = length(vPosition);
		float zmax = rDepth * zminDepth;
		UV.y = 1.0 - log(z/zminDepth) / log(zmax/zminDepth);
	}
	
	UV = clamp(UV, 0.01, 0.99);
	gl_FragColor = texture2D(myTextureSampler, UV);
}
