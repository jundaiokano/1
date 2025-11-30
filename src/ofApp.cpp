#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    // Set window size explicitly
    ofSetWindowShape(1280, 720);

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

    // Auto-sequence
    autoSequenceMode = false;
    sequenceStartTime = 0;
    currentSequencePhase = 0;

    // Audio analysis
    bass = mid = treble = beat = 0.0;
    lastBeatTime = 0;
    fftSmoothed.resize(FFT_SIZE / 2, 0.0);

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

    // Resize image to fit window while maintaining aspect ratio
    float windowW = ofGetWidth();
    float windowH = ofGetHeight();
    float imgW = targetImg.getWidth();
    float imgH = targetImg.getHeight();

    // Calculate scale to fit image in window (with some margin)
    float margin = 50;
    float availableW = windowW - margin * 2;
    float availableH = windowH - margin * 2;

    float scaleX = availableW / imgW;
    float scaleY = availableH / imgH;
    float scale = min(scaleX, scaleY);

    int newW = imgW * scale;
    int newH = imgH * scale;

    targetImg.resize(newW, newH);

    ofLog() << "Image loaded and resized to: " << newW << "x" << newH;
    ofLog() << "Window size: " << windowW << "x" << windowH;

    // Initialize particles from image
    initParticlesFromImage();

    // Setup VBO Mesh
    mesh.setMode(OF_PRIMITIVE_POINTS);
    mesh.enableColors();

    // Enable point sprites for better looking particles
    glEnable(GL_POINT_SMOOTH);
    glPointSize(2.0);

    // Initialize bird formations
    initBirdFormations();

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
    // Analyze audio (FFT, beat detection)
    analyzeAudio();

    // Smooth the volume
    scaledVol = ofClamp(smoothedVol * 3.0, 0.0, 1.0);

    // Update auto-sequence if enabled
    if (autoSequenceMode) {
        updateAutoSequence();
    }

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
        case 4:
            updateMode4_BirdFormation();
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
// Mode 4: Bird Formation - Particles form flying bird shapes
//--------------------------------------------------------------
void ofApp::updateMode4_BirdFormation() {
    float time = ofGetElapsedTimef();

    // Update bird flock positions and wing flapping
    for (int i = 0; i < birdFlocks.size(); i++) {
        auto& bird = birdFlocks[i];

        // Move bird across screen
        bird.center += bird.velocity * (1.0 + bass * 0.5);  // Bass affects flight speed

        // Wing flapping (affected by beat)
        bird.wingPhase += 0.1 + beat * 0.3;

        // Wrap around screen
        if (bird.center.x < -200) bird.center.x = ofGetWidth() + 200;
        if (bird.center.x > ofGetWidth() + 200) bird.center.x = -200;
        if (bird.center.y < -200) bird.center.y = ofGetHeight() + 200;
        if (bird.center.y > ofGetHeight() + 200) bird.center.y = -200;

        // Gentle wave motion (up and down)
        float wave = sin(time * 0.5 + i) * 30.0;
        bird.center.y += wave * 0.01;

        // Regenerate bird shape with current wing phase
        generateBirdShape(i);
    }

    // Move particles towards their bird positions
    for (auto& p : particles) {
        if (!p.isActive) continue;

        // Seek bird position with easing
        glm::vec3 toBird = p.birdPos - p.pos;
        float dist = glm::length(toBird);

        // Easing factor (tunable)
        float easing = 0.08;

        // Apply seeking force
        p.vel += toBird * easing;

        // Apply friction
        p.vel *= 0.88;

        p.pos += p.vel;

        // Add slight turbulence for organic feel
        float turbulence = 0.3;
        p.pos.x += ofNoise(p.uniqueVal, time) * turbulence - turbulence * 0.5;
        p.pos.y += ofNoise(p.uniqueVal + 100, time) * turbulence - turbulence * 0.5;
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
    string modeNames[] = {"Gathering", "Pulse", "Organism", "Reconstruct", "Bird Formation"};
    string info = "";
    info += "FPS: " + ofToString(ofGetFrameRate(), 1) + "\n";
    info += "Mode: " + ofToString(mode + 1) + " - " + modeNames[mode] + "\n";

    if (autoSequenceMode) {
        float elapsed = ofGetElapsedTimef() - sequenceStartTime;
        info += "AUTO-SEQUENCE: Phase " + ofToString(currentSequencePhase) + " (" + ofToString(elapsed, 1) + "s)\n";
    }

    info += "Active Particles: " + ofToString(mesh.getNumVertices()) + " / " + ofToString(targetParticleCount) + "\n";
    info += "Audio - Volume: " + ofToString(scaledVol, 2);
    info += " | Bass: " + ofToString(bass, 2);
    info += " | Mid: " + ofToString(mid, 2);
    info += " | Treble: " + ofToString(treble, 2) + "\n";
    info += "Beat: " + ofToString(beat, 2) + "\n";
    info += "Image: " + ofToString(targetImg.getWidth()) + "x" + ofToString(targetImg.getHeight()) + " | ";
    info += "Window: " + ofToString(ofGetWidth()) + "x" + ofToString(ofGetHeight()) + "\n";
    info += "\n";
    info += "Controls:\n";
    info += "1-5: Switch Mode (1=Gathering, 2=Pulse, 3=Organism, 4=Reconstruct, 5=Birds)\n";
    info += "6: Start Auto-Sequence (60s loop)\n";
    info += "S: Stop Auto-Sequence\n";
    info += "R: Reset Particles\n";
    info += "Mouse: Spawn Particles";

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
// Initialize bird formations
//--------------------------------------------------------------
void ofApp::initBirdFormations() {
    birdFlocks.clear();

    // Create NUM_BIRDS bird flocks at different positions
    for (int i = 0; i < NUM_BIRDS; i++) {
        BirdFlock bird;

        // Random starting position
        bird.center = glm::vec3(
            ofRandom(200, ofGetWidth() - 200),
            ofRandom(100, ofGetHeight() / 2),  // Upper half of screen
            ofRandom(-50, 50)
        );

        // Random velocity (mostly horizontal)
        float angle = ofRandom(-PI / 6, PI / 6);  // Slight angle variation
        float speed = ofRandom(1.0, 2.5);
        bird.velocity = glm::vec3(cos(angle) * speed, sin(angle) * speed, 0);

        bird.wingPhase = ofRandom(TWO_PI);
        bird.size = ofRandom(80, 150);  // Size variation

        birdFlocks.push_back(bird);
    }

    // Assign particles to bird groups
    generateBirdShape(0);  // Initial generation
}

//--------------------------------------------------------------
// Generate bird silhouette shape mathematically
//--------------------------------------------------------------
void ofApp::generateBirdShape(int birdIndex) {
    if (birdIndex >= birdFlocks.size()) return;

    const auto& bird = birdFlocks[birdIndex];

    // Calculate particles per bird
    int particlesPerBird = particles.size() / NUM_BIRDS;
    int startIdx = birdIndex * particlesPerBird;
    int endIdx = (birdIndex == NUM_BIRDS - 1) ? particles.size() : (birdIndex + 1) * particlesPerBird;

    // V-shaped bird silhouette (simplified)
    int count = 0;
    for (int i = startIdx; i < endIdx; i++) {
        auto& p = particles[i];
        p.birdGroup = birdIndex;

        // Create V-shape using parametric equations
        float t = (float)(i - startIdx) / (endIdx - startIdx);  // 0 to 1

        // Wing flapping amplitude
        float wingFlap = sin(bird.wingPhase) * 15.0;

        float x, y;

        if (t < 0.05) {
            // Head (small cluster)
            x = 0;
            y = 0;
        } else if (t < 0.5) {
            // Left wing
            float wingT = (t - 0.05) / 0.45;
            x = -wingT * bird.size;
            y = wingT * bird.size * 0.7 + wingFlap * wingT;
        } else {
            // Right wing
            float wingT = (t - 0.5) / 0.5;
            x = wingT * bird.size;
            y = wingT * bird.size * 0.7 + wingFlap * wingT;
        }

        // Add body thickness
        float bodyThickness = ofRandom(-5, 5);
        x += bodyThickness;

        // Apply to particle's bird position
        p.birdPos = bird.center + glm::vec3(x, y, 0);

        // Color variation (slightly lighter for sky creatures)
        float brightness = ofMap(t, 0, 1, 0.8, 1.2);
        p.color.r = ofClamp(p.color.r * brightness, 0, 1);
        p.color.g = ofClamp(p.color.g * brightness, 0, 1);
        p.color.b = ofClamp(p.color.b * brightness, 0, 1);

        count++;
    }
}

//--------------------------------------------------------------
// Auto-sequence system - automatic mode transitions
//--------------------------------------------------------------
void ofApp::startAutoSequence() {
    autoSequenceMode = true;
    sequenceStartTime = ofGetElapsedTimef();
    currentSequencePhase = 0;
    mode = 0;
    ofLog() << "Auto-sequence started";
}

void ofApp::updateAutoSequence() {
    float elapsed = ofGetElapsedTimef() - sequenceStartTime;

    // Sequence timing (in seconds) - tunable
    float phase0End = 10;   // 0-10s: Gathering
    float phase1End = 20;   // 10-20s: Pulse
    float phase2End = 35;   // 20-35s: Bird Formation
    float phase3End = 45;   // 35-45s: Organism
    float phase4End = 60;   // 45-60s: Reconstruct

    if (elapsed < phase0End && mode != 0) {
        mode = 0;
        currentSequencePhase = 0;
        ofLog() << "Auto-sequence: Phase 0 - Gathering";
    } else if (elapsed >= phase0End && elapsed < phase1End && mode != 1) {
        mode = 1;
        currentSequencePhase = 1;
        ofLog() << "Auto-sequence: Phase 1 - Pulse";
    } else if (elapsed >= phase1End && elapsed < phase2End && mode != 4) {
        mode = 4;
        currentSequencePhase = 2;
        ofLog() << "Auto-sequence: Phase 2 - Bird Formation";
    } else if (elapsed >= phase2End && elapsed < phase3End && mode != 2) {
        mode = 2;
        currentSequencePhase = 3;
        ofLog() << "Auto-sequence: Phase 3 - Organism (Dispersion)";
    } else if (elapsed >= phase3End && elapsed < phase4End && mode != 3) {
        mode = 3;
        currentSequencePhase = 4;
        ofLog() << "Auto-sequence: Phase 4 - Reconstruct";
    } else if (elapsed >= phase4End) {
        // Loop or stop
        autoSequenceMode = false;
        ofLog() << "Auto-sequence completed";
    }
}

//--------------------------------------------------------------
// Audio analysis with FFT
//--------------------------------------------------------------
void ofApp::analyzeAudio() {
    if (lastBuffer.size() == 0) return;

    // Perform simple FFT using ofSoundBuffer
    // Note: openFrameworks doesn't have built-in FFT, so we'll use a simplified approach
    // For full FFT, you'd typically use ofxFft addon, but we're avoiding addons

    // Simple frequency band analysis using time-domain approximation
    int numFrames = lastBuffer.getNumFrames();
    if (numFrames == 0) return;

    float lowSum = 0, midSum = 0, highSum = 0;
    int lowCount = 0, midCount = 0, highCount = 0;

    // Divide buffer into frequency-like bands (approximation)
    for (int i = 0; i < numFrames; i++) {
        float sample = abs(lastBuffer[i * lastBuffer.getNumChannels()]);

        // Low frequencies (first third)
        if (i < numFrames / 3) {
            lowSum += sample;
            lowCount++;
        }
        // Mid frequencies (second third)
        else if (i < numFrames * 2 / 3) {
            midSum += sample;
            midCount++;
        }
        // High frequencies (last third)
        else {
            highSum += sample;
            highCount++;
        }
    }

    // Average and smooth
    float newBass = (lowCount > 0) ? lowSum / lowCount : 0;
    float newMid = (midCount > 0) ? midSum / midCount : 0;
    float newTreble = (highCount > 0) ? highSum / highCount : 0;

    bass = bass * 0.8 + newBass * 0.2;
    mid = mid * 0.8 + newMid * 0.2;
    treble = treble * 0.8 + newTreble * 0.2;

    // Detect beat
    detectBeat();
}

//--------------------------------------------------------------
// Simple beat detection
//--------------------------------------------------------------
void ofApp::detectBeat() {
    float currentTime = ofGetElapsedTimef();

    // Beat threshold (tunable)
    float beatThreshold = 0.3;
    float minBeatInterval = 0.2;  // Minimum 200ms between beats

    // Detect sudden increase in bass
    if (bass > beatThreshold && (currentTime - lastBeatTime) > minBeatInterval) {
        beat = 1.0;
        lastBeatTime = currentTime;
    } else {
        // Decay beat intensity
        beat *= 0.9;
    }
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer & input) {
    // Store buffer for analysis
    lastBuffer = input;

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
            autoSequenceMode = false;
            ofLog() << "Mode: 0 - Gathering";
            break;
        case '2':
            mode = 1;
            autoSequenceMode = false;
            ofLog() << "Mode: 1 - Pulse";
            break;
        case '3':
            mode = 2;
            autoSequenceMode = false;
            ofLog() << "Mode: 2 - Organism";
            break;
        case '4':
            mode = 3;
            autoSequenceMode = false;
            ofLog() << "Mode: 3 - Reconstruct";
            break;
        case '5':
            mode = 4;
            autoSequenceMode = false;
            ofLog() << "Mode: 4 - Bird Formation";
            break;
        case '6':
            startAutoSequence();
            break;
        case 'r':
        case 'R':
            resetParticles();
            ofLog() << "Particles reset";
            break;
        case 's':
        case 'S':
            // Stop auto-sequence
            autoSequenceMode = false;
            ofLog() << "Auto-sequence stopped";
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
