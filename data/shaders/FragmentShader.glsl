#version 460 core

out vec4 FragColor;
in vec3 vLocalPos;

// Binding 0: RGBA8 (RGB=Normal, A=Density)
layout(binding = 0) uniform sampler3D u_VolumeTexture;
// Binding 1: Transfer Function
layout(binding = 1) uniform sampler1D u_TransferFunction;

uniform vec3 u_CameraPos;
uniform mat4 u_ModelMatrix;
uniform vec3 u_LightDir;

uniform float u_StepSize;       // Шаг луча
uniform float u_DensityThreshold; 
uniform float u_OpacityMultiplier;

// При шаге 0.002 это позволяет пройти дистанцию 6.0 единиц (весь объем с запасом)
const int MAX_SAFE_STEPS = 3000; 

// AABB Intersection
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
    
    // 1. Границы объема
    vec2 tHit = intersectAABB(localCamPos, rayDir, vec3(-1.0), vec3(1.0));
    float tNear = tHit.x;
    float tFar = tHit.y;
    
    if (tNear > tFar || tFar < 0.0) discard;
    tNear = max(tNear, 0.0); 

    // Jittering
    tNear += random(gl_FragCoord.xy) * u_StepSize;

    vec3 currentPos = localCamPos + rayDir * tNear;
    vec4 accumulatedColor = vec4(0.0);
    
    // Текущая дистанция от камеры
    float currentDist = tNear;
    
    // Счетчик шагов только для защиты от зависания (Watchdog)
    int stepsTaken = 0;
    
    vec3 ambientLight = vec3(0.25); 

    // --- ГЛАВНЫЙ ЦИКЛ ---
    // Условие: Пока луч физически находится внутри куба
    while (currentDist < tFar) {
        
        // 0. Защита от бесконечного цикла (TDR crash protection)
        if (stepsTaken > MAX_SAFE_STEPS) break;
        stepsTaken++;

        // 1. Early Ray Termination (если уже непрозрачно)
        if(accumulatedColor.a >= 0.98) break; 
        
        vec3 texCoord = (currentPos + 1.0) * 0.5;
        
        // Чтение данных (Alpha = Density)
        float density = texture(u_VolumeTexture, texCoord).a;

        if (texCoord.x < 0.01 || texCoord.x > 0.99 ||
            texCoord.y < 0.01 || texCoord.y > 0.99 ||
            texCoord.z < 0.01 || texCoord.z > 0.99) 
        {
            density = 0.0;
        }

        // 2. Empty Space Skipping
        if (density < u_DensityThreshold) {
            // Пустота -> большой шаг
            float skipStep = u_StepSize * 4.0;
            
            // Важно: не перепрыгнуть tFar
            if (currentDist + skipStep > tFar) {
                // Если прыжок вылетает за границы, просто прерываемся
                break; 
            }

            currentPos += rayDir * skipStep;
            currentDist += skipStep;
            continue; 
        }

        // 3. Обработка "тела"
        vec4 srcColor = texture(u_TransferFunction, density);
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
        
        // Стандартный шаг
        currentPos += rayDir * u_StepSize;
        currentDist += u_StepSize;
    }
    
    FragColor = accumulatedColor;
}