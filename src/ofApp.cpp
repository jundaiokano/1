#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(0);
    ofEnableAlphaBlending();
    ofSetCircleResolution(64);

    // Initialize variables
    mode = 0;
    smoothedVol = 0.0;
    scaledVol = 0.0;
    spawnIndex = 0;
    isMousePressed = false;
    stride = 2;  // Sample every 2 pixels (tunable for performance)

    // Setup Audio Input
    ofSoundStreamSettings settings;
    auto devices = soundStream.getMatchingDevices("default");
    if (!devices.empty()) {
        settings.setInDevice(devices[0]);
    }
    settings.setInListener(this);
    settings.sampleRate = 44100;
    settings.numOutputChannels = 0;
    settings.numInputChannels = 2;
    settings.bufferSize = 256;
    soundStream.setup(settings);

    // Load target image
    bool imageLoaded = targetImg.load("image.jpg");

    if (!imageLoaded) {
        // Generate placeholder image if loading fails
        ofLog() << "Warning: Could not load image.jpg, generating placeholder";
        int w = 800;
        int h = 600;
        targetImg.allocate(w, h, OF_IMAGE_COLOR);

        // Create gradient placeholder
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float noise = ofNoise(x * 0.01, y * 0.01);
                float hue = ofMap(x, 0, w, 0, 255);
                float brightness = ofMap(y, 0, h, 100, 255);
                ofColor col = ofColor::fromHsb(hue, 200, brightness * noise);
                targetImg.setColor(x, y, col);
            }
        }
        targetImg.update();
    }

    // Initialize particles from image
    initParticlesFromImage();

    // Setup VBO Mesh
    mesh.setMode(OF_PRIMITIVE_POINTS);
    mesh.enableColors();

    // Enable point sprites for better looking particles
    glEnable(GL_POINT_SMOOTH);
    glPointSize(2.0);

    ofLog() << "Setup complete. Total particles: " << particles.size();
}

//--------------------------------------------------------------
void ofApp::initParticlesFromImage() {
    particles.clear();

    int w = targetImg.getWidth();
    int h = targetImg.getHeight();

    // Calculate offset to center the image reconstruction on screen
    float offsetX = (ofGetWidth() - w) / 2.0;
    float offsetY = (ofGetHeight() - h) / 2.0;

    // Sample pixels with stride to control particle count
    for (int y = 0; y < h; y += stride) {
        for (int x = 0; x < w; x += stride) {
            if (particles.size() >= MAX_PARTICLES) break;

            Particle p;

            // Target position (where it should reconstruct)
            p.targetPos = glm::vec3(x + offsetX, y + offsetY, 0);

            // Sample color from image
            ofColor col = targetImg.getColor(x, y);
            p.color = ofFloatColor(col.r / 255.0, col.g / 255.0, col.b / 255.0, 1.0);

            // Start at random off-screen position
            p.pos = glm::vec3(
                ofRandom(-200, ofGetWidth() + 200),
                ofRandom(-200, ofGetHeight() + 200),
                ofRandom(-100, 100)
            );

            // Random velocity
            p.vel = glm::vec3(
                ofRandom(-1, 1),
                ofRandom(-1, 1),
                ofRandom(-0.5, 0.5)
            );

            p.uniqueVal = ofRandom(1000.0);
            p.isActive = false;  // Start inactive, spawn gradually

            particles.push_back(p);
        }
        if (particles.size() >= MAX_PARTICLES) break;
    }

    targetParticleCount = particles.size();
}

//--------------------------------------------------------------
void ofApp::update() {
    // Smooth the volume
    scaledVol = ofClamp(smoothedVol * 3.0, 0.0, 1.0);

    // Spawn particles if mouse is pressed (for testing/interaction)
    if (isMousePressed) {
        spawnParticles(100);  // Spawn 100 particles per frame when mouse pressed
    } else {
        spawnParticles(30);  // Gradual spawn
    }

    // Update particles based on current mode
    updateParticles();

    // Update mesh for rendering
    updateMesh();
}

//--------------------------------------------------------------
void ofApp::updateParticles() {
    switch (mode) {
        case 0:
            updateMode0_Gathering();
            break;
        case 1:
            updateMode1_Pulse();
            break;
        case 2:
            updateMode2_Organism();
            break;
        case 3:
            updateMode3_Reconstruct();
            break;
    }
}

//--------------------------------------------------------------
// Mode 0: Gathering - Gentle Perlin Noise flow
//--------------------------------------------------------------
void ofApp::updateMode0_Gathering() {
    float time = ofGetElapsedTimef();

    for (auto& p : particles) {
        if (!p.isActive) continue;

        // Perlin noise-based flow field
        float noiseScale = 0.003;  // Tunable: smaller = smoother flow
        float timeScale = 0.3;     // Tunable: speed of noise evolution
        float forceStrength = 0.5; // Tunable: movement strength

        float noiseX = ofNoise(
            p.pos.x * noiseScale + p.uniqueVal,
            p.pos.y * noiseScale,
            time * timeScale
        ) * 2.0 - 1.0;

        float noiseY = ofNoise(
            p.pos.x * noiseScale,
            p.pos.y * noiseScale + p.uniqueVal,
            time * timeScale + 100.0
        ) * 2.0 - 1.0;

        float noiseZ = ofNoise(
            p.pos.x * noiseScale,
            p.pos.y * noiseScale,
            time * timeScale + 200.0 + p.uniqueVal
        ) * 2.0 - 1.0;

        glm::vec3 force = glm::vec3(noiseX, noiseY, noiseZ) * forceStrength;

        // Apply force
        p.vel += force;
        p.vel *= 0.95;  // Friction

        p.pos += p.vel;

        // Wrap around screen edges
        if (p.pos.x < -100) p.pos.x = ofGetWidth() + 100;
        if (p.pos.x > ofGetWidth() + 100) p.pos.x = -100;
        if (p.pos.y < -100) p.pos.y = ofGetHeight() + 100;
        if (p.pos.y > ofGetHeight() + 100) p.pos.y = -100;
    }
}

//--------------------------------------------------------------
// Mode 1: Pulse - Attraction to center with audio reaction
//--------------------------------------------------------------
void ofApp::updateMode1_Pulse() {
    glm::vec3 center = glm::vec3(ofGetWidth() / 2.0, ofGetHeight() / 2.0, 0);

    for (auto& p : particles) {
        if (!p.isActive) continue;

        // Vector from particle to center
        glm::vec3 toCenter = center - p.pos;
        float dist = glm::length(toCenter);

        if (dist > 0.1) {
            // Normalize and apply attraction
            glm::vec3 direction = glm::normalize(toCenter);

            // Audio-reactive force
            float baseAttraction = 0.3;  // Tunable: base attraction strength
            float audioBoost = scaledVol * 2.0;  // Audio multiplier

            // Pulse effect: push away when volume is high
            float pulseForce = scaledVol * 5.0;

            if (scaledVol > 0.5) {
                // High volume: push outward
                p.vel -= direction * pulseForce;
            } else {
                // Low volume: attract inward
                p.vel += direction * (baseAttraction + audioBoost);
            }
        }

        // Apply velocity with friction
        p.vel *= 0.92;
        p.pos += p.vel;

        // Soft boundaries
        float margin = 50;
        if (p.pos.x < margin) p.vel.x += 0.5;
        if (p.pos.x > ofGetWidth() - margin) p.vel.x -= 0.5;
        if (p.pos.y < margin) p.vel.y += 0.5;
        if (p.pos.y > ofGetHeight() - margin) p.vel.y -= 0.5;
    }
}

//--------------------------------------------------------------
// Mode 2: Organism - Boids/Swarm behavior with Curl Noise
//--------------------------------------------------------------
void ofApp::updateMode2_Organism() {
    float time = ofGetElapsedTimef();

    for (auto& p : particles) {
        if (!p.isActive) continue;

        // Curl noise for organic flow
        glm::vec3 curl = getCurlNoise(p.pos, time + p.uniqueVal);

        // Audio-reactive speed
        float speedMultiplier = 1.0 + scaledVol * 2.0;  // Tunable

        p.vel += curl * speedMultiplier;
        p.vel *= 0.88;  // Friction

        // Limit max speed
        float maxSpeed = 3.0 + scaledVol * 5.0;
        if (glm::length(p.vel) > maxSpeed) {
            p.vel = glm::normalize(p.vel) * maxSpeed;
        }

        p.pos += p.vel;

        // Wrap around screen edges
        if (p.pos.x < -100) p.pos.x = ofGetWidth() + 100;
        if (p.pos.x > ofGetWidth() + 100) p.pos.x = -100;
        if (p.pos.y < -100) p.pos.y = ofGetHeight() + 100;
        if (p.pos.y > ofGetHeight() + 100) p.pos.y = -100;
    }
}

//--------------------------------------------------------------
// Mode 3: Reconstruct - Move towards target image positions
//--------------------------------------------------------------
void ofApp::updateMode3_Reconstruct() {
    for (auto& p : particles) {
        if (!p.isActive) continue;

        // Seek target position with easing
        glm::vec3 toTarget = p.targetPos - p.pos;
        float dist = glm::length(toTarget);

        // Easing factor (tunable: 0.01 = slow, 0.1 = fast)
        float easing = 0.05;

        // Apply seeking force
        p.vel += toTarget * easing;

        // Apply friction to settle at target
        p.vel *= 0.85;

        p.pos += p.vel;

        // Stop tiny movements when close to target
        if (dist < 0.5) {
            p.vel *= 0.5;
        }
    }
}

//--------------------------------------------------------------
void ofApp::updateMesh() {
    mesh.clear();

    for (const auto& p : particles) {
        if (!p.isActive) continue;

        mesh.addVertex(p.pos);

        // Color with audio-reactive brightness boost
        float brightnessBoost = 1.0 + scaledVol * 0.5;
        ofFloatColor col = p.color;
        col.r *= brightnessBoost;
        col.g *= brightnessBoost;
        col.b *= brightnessBoost;

        mesh.addColor(col);
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(0);

    // Enable additive blending for glowy effect
    ofEnableBlendMode(OF_BLENDMODE_ADD);

    // Draw particles
    mesh.draw();

    // Reset blend mode for UI
    ofDisableBlendMode();
    ofEnableAlphaBlending();

    // Draw debug info
    ofSetColor(255);
    string modeNames[] = {"Gathering", "Pulse", "Organism", "Reconstruct"};
    string info = "";
    info += "FPS: " + ofToString(ofGetFrameRate(), 1) + "\n";
    info += "Mode: " + ofToString(mode + 1) + " - " + modeNames[mode] + "\n";
    info += "Active Particles: " + ofToString(mesh.getNumVertices()) + " / " + ofToString(targetParticleCount) + "\n";
    info += "Audio Level: " + ofToString(scaledVol, 2) + "\n";
    info += "\n";
    info += "Controls:\n";
    info += "1-4: Switch Mode\n";
    info += "R: Reset Particles\n";
    info += "Mouse Click: Spawn Particles";

    ofDrawBitmapString(info, 20, 20);
}

//--------------------------------------------------------------
void ofApp::spawnParticles(int count) {
    for (int i = 0; i < count; i++) {
        if (spawnIndex >= particles.size()) {
            break;  // All particles already spawned
        }

        particles[spawnIndex].isActive = true;
        spawnIndex++;
    }
}

//--------------------------------------------------------------
void ofApp::resetParticles() {
    for (auto& p : particles) {
        // Reset to random off-screen position
        p.pos = glm::vec3(
            ofRandom(-200, ofGetWidth() + 200),
            ofRandom(-200, ofGetHeight() + 200),
            ofRandom(-100, 100)
        );

        p.vel = glm::vec3(
            ofRandom(-1, 1),
            ofRandom(-1, 1),
            ofRandom(-0.5, 0.5)
        );

        p.isActive = false;
    }

    spawnIndex = 0;
}

//--------------------------------------------------------------
// Curl Noise calculation for organic flow
//--------------------------------------------------------------
glm::vec3 ofApp::getCurlNoise(glm::vec3 p, float t) {
    float eps = 0.1;  // Epsilon for numerical derivative
    float scale = 0.005;  // Noise scale (tunable)

    // Sample noise at neighboring points
    float n1 = ofNoise((p.x) * scale, (p.y + eps) * scale, (p.z) * scale + t);
    float n2 = ofNoise((p.x) * scale, (p.y - eps) * scale, (p.z) * scale + t);
    float n3 = ofNoise((p.x) * scale, (p.y) * scale, (p.z + eps) * scale + t);
    float n4 = ofNoise((p.x) * scale, (p.y) * scale, (p.z - eps) * scale + t);

    // Calculate curl (rotational field)
    float a = (n1 - n2) / (2.0 * eps);
    float b = (n3 - n4) / (2.0 * eps);

    return glm::vec3(b, -a, a - b) * 0.3;  // Scale factor (tunable)
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer & input) {
    // Calculate RMS (Root Mean Square) volume
    float sum = 0.0;
    for (size_t i = 0; i < input.getNumFrames(); i++) {
        float sample = input[i * input.getNumChannels()];
        sum += sample * sample;
    }

    float rms = sqrt(sum / input.getNumFrames());

    // Smooth the volume with simple low-pass filter
    smoothedVol = smoothedVol * 0.9 + rms * 0.1;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    switch (key) {
        case '1':
            mode = 0;
            ofLog() << "Mode: 0 - Gathering";
            break;
        case '2':
            mode = 1;
            ofLog() << "Mode: 1 - Pulse";
            break;
        case '3':
            mode = 2;
            ofLog() << "Mode: 2 - Organism";
            break;
        case '4':
            mode = 3;
            ofLog() << "Mode: 3 - Reconstruct";
            break;
        case 'r':
        case 'R':
            resetParticles();
            ofLog() << "Particles reset";
            break;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
    isMousePressed = true;
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
    isMousePressed = false;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
}
