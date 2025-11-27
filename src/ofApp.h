#pragma once

#include "ofMain.h"

// ========================================
// Particle Structure
// ========================================
struct Particle {
    glm::vec3 pos;        // Current position
    glm::vec3 vel;        // Velocity
    glm::vec3 targetPos;  // Target position (from image pixel)
    ofFloatColor color;   // Color sampled from image
    float uniqueVal;      // Random offset for Perlin noise
    bool isActive;        // Is this particle spawned/visible?

    Particle() {
        pos = glm::vec3(0, 0, 0);
        vel = glm::vec3(0, 0, 0);
        targetPos = glm::vec3(0, 0, 0);
        color = ofFloatColor(1.0, 1.0, 1.0);
        uniqueVal = ofRandom(1000.0);
        isActive = false;
    }
};

// ========================================
// Main Application Class
// ========================================
class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

    void audioIn(ofSoundBuffer & input);

private:
    // Core Data
    vector<Particle> particles;
    ofVboMesh mesh;
    ofImage targetImg;

    // Mode & State
    int mode;  // 0=Gathering, 1=Pulse, 2=Organism, 3=Reconstruct
    int targetParticleCount;

    // Audio
    ofSoundStream soundStream;
    float smoothedVol;
    float scaledVol;
    vector<float> volHistory;

    // Spawn Control
    int spawnIndex;
    bool isMousePressed;

    // Performance Settings
    static const int MAX_PARTICLES = 30000;
    int stride;  // Pixel sampling stride

    // Helper Methods
    void initParticlesFromImage();
    void updateParticles();
    void updateMesh();
    void resetParticles();
    void spawnParticles(int count);

    // Mode-specific update functions
    void updateMode0_Gathering();
    void updateMode1_Pulse();
    void updateMode2_Organism();
    void updateMode3_Reconstruct();

    // Utility
    glm::vec3 getCurlNoise(glm::vec3 p, float t);
};
