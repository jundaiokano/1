#pragma once

#include "ofMain.h"

// ========================================
// Particle Structure
// ========================================
struct Particle {
    glm::vec3 pos;        // Current position
    glm::vec3 vel;        // Velocity
    glm::vec3 targetPos;  // Target position (from image pixel)
    glm::vec3 birdPos;    // Target position for bird formation
    ofFloatColor color;   // Color sampled from image
    float uniqueVal;      // Random offset for Perlin noise
    bool isActive;        // Is this particle spawned/visible?
    int birdGroup;        // Which bird flock this particle belongs to (0-N)

    Particle() {
        pos = glm::vec3(0, 0, 0);
        vel = glm::vec3(0, 0, 0);
        targetPos = glm::vec3(0, 0, 0);
        birdPos = glm::vec3(0, 0, 0);
        color = ofFloatColor(1.0, 1.0, 1.0);
        uniqueVal = ofRandom(1000.0);
        isActive = false;
        birdGroup = 0;
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
    int mode;  // 0=Gathering, 1=Pulse, 2=Organism, 3=Reconstruct, 4=BirdFormation
    int targetParticleCount;

    // Auto-sequence Mode
    bool autoSequenceMode;
    float sequenceStartTime;
    int currentSequencePhase;

    // Audio
    ofSoundStream soundStream;
    float smoothedVol;
    float scaledVol;
    vector<float> volHistory;

    // FFT Audio Analysis
    ofSoundBuffer lastBuffer;
    vector<float> fftSmoothed;
    float bass, mid, treble;  // Frequency bands
    float beat;               // Beat detection
    float lastBeatTime;
    static const int FFT_SIZE = 512;

    // Spawn Control
    int spawnIndex;
    bool isMousePressed;

    // Bird Formation
    static const int NUM_BIRDS = 5;
    struct BirdFlock {
        glm::vec3 center;
        glm::vec3 velocity;
        float wingPhase;
        float size;
    };
    vector<BirdFlock> birdFlocks;

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
    void updateMode4_BirdFormation();

    // Auto-sequence
    void updateAutoSequence();
    void startAutoSequence();

    // Bird formation
    void initBirdFormations();
    void generateBirdShape(int birdIndex);

    // Audio analysis
    void analyzeAudio();
    void detectBeat();

    // Utility
    glm::vec3 getCurlNoise(glm::vec3 p, float t);
};
