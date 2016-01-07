uniform vec4 ambientColor;
uniform vec4 diffuseColor;   
uniform vec4 specularColor;
uniform vec4 kblue;
uniform vec4 kyellow;
uniform float alpha;
uniform float beta;
 
varying vec3 N;
varying vec3 v;

void main (void)  
{ 
	// Light direction	
	vec3 L = normalize(gl_LightSource[0].position.xyz - v);   

	// Implement Gooch equation
	vec4 kcool = kblue + (alpha * diffuseColor);
	vec4 kwarm = kyellow + (beta * diffuseColor);

	float LdotN = dot(N,L);
	float value2 = (1.0 + LdotN) / 2.0;
	float value = 1.0 - value2;
	
	vec4 Idiff = (value * kcool) + (value2 * kwarm);
	Idiff = clamp(Idiff, 0.0, 1.0);
	
	gl_FragColor = 0.5 * Idiff;
	
	
	
// rest of phong illumination (ambient and specular light).
	  
   vec3 E = normalize(-v); // eye at (0,0,0)  
   vec3 R = normalize(reflect(-L,N));  
 
   // Ambient light 
//   vec4 Iamb = ambientColor;     
   
   // Specular light
   vec4 Ispec = specularColor * pow(max(dot(R,E),0.0),64.0);
   Ispec = clamp(Ispec, 0.0, 1.0);
   
   gl_FragColor = 0.5 * (Idiff + Ispec); 


// method to draw outlines 
/*
	float outline = dot(normalize(-v), N);
	if (outline < 0.2)
		gl_FragColor = vec4(0.0,0.0,0.0,1.0);
*/
  
}
