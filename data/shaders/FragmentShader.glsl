#version 300 es
precision highp float;
precision highp sampler3D;

out vec4 FragColor;
in vec3 vLocalPos;

uniform sampler3D u_VolumeTexture;
// WebGL 2 не поддерживает sampler1D, используем sampler2D (Nx1)
uniform sampler2D u_TransferFunction;

uniform vec3 u_CameraPos;
uniform mat4 u_ModelMatrix;
uniform vec3 u_LightDir;

uniform float u_StepSize;
uniform float u_DensityThreshold; 
uniform float u_OpacityMultiplier;

const int MAX_SAFE_STEPS = 1000; 

vec2 intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) {
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

float random(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    mat4 invModel = inverse(u_ModelMatrix);
    vec3 localCamPos = vec3(invModel * vec4(u_CameraPos, 1.0));
    vec3 rayDir = normalize(vLocalPos - localCamPos);
    
    vec2 tHit = intersectAABB(localCamPos, rayDir, vec3(-1.0), vec3(1.0));
    float tNear = tHit.x;
    float tFar = tHit.y;
    
    if (tNear > tFar || tFar < 0.0) discard;
    tNear = max(tNear, 0.0); 

    tNear += random(gl_FragCoord.xy) * u_StepSize;

    vec3 currentPos = localCamPos + rayDir * tNear;
    vec4 accumulatedColor = vec4(0.0);
    float currentDist = tNear;
    int stepsTaken = 0;
    
    vec3 ambientLight = vec3(0.25); 

    while (currentDist < tFar) {
        if (stepsTaken > MAX_SAFE_STEPS) break;
        stepsTaken++;

        if(accumulatedColor.a >= 0.98) break; 
        
        vec3 texCoord = (currentPos + 1.0) * 0.5;
        
        float density = texture(u_VolumeTexture, texCoord).a;

        if (texCoord.x < 0.01 || texCoord.x > 0.99 ||
            texCoord.y < 0.01 || texCoord.y > 0.99 ||
            texCoord.z < 0.01 || texCoord.z > 0.99) 
        {
            density = 0.0;
        }

        if (density < u_DensityThreshold) {
            float skipStep = u_StepSize * 4.0;
            if (currentDist + skipStep > tFar) break; 
            currentPos += rayDir * skipStep;
            currentDist += skipStep;
            continue; 
        }

        // Выборка из 2D текстуры (y=0.5) вместо 1D
        vec4 srcColor = texture(u_TransferFunction, vec2(density, 0.5));
        
        srcColor.a *= u_OpacityMultiplier;
        srcColor.a = clamp(srcColor.a, 0.0, 0.95);

        if (srcColor.a > 0.01) {
            vec3 normal = normalize(texture(u_VolumeTexture, texCoord).rgb * 2.0 - 1.0);
            
            float diff = max(dot(normal, u_LightDir), 0.0);
            vec3 diffuse = diff * vec3(0.8);
            
            srcColor.rgb *= (ambientLight + diffuse);
            srcColor.a = 1.0 - pow(1.0 - srcColor.a, u_StepSize * 150.0);
            srcColor.rgb *= srcColor.a;
            accumulatedColor = accumulatedColor + srcColor * (1.0 - accumulatedColor.a);
        }
        
        currentPos += rayDir * u_StepSize;
        currentDist += u_StepSize;
    }
    
    FragColor = accumulatedColor;
}