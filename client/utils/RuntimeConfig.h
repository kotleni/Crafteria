#ifndef RUNTIMECONFIG_H
#define RUNTIMECONFIG_H

struct RuntimeConfig {
  int maxRenderingDistance;
  bool isMouseRelative;
  bool isEnableVsync;
  bool isChunkGenerationEnabled;
  bool isChunkBakingEnabled;
};

#endif //RUNTIMECONFIG_H
