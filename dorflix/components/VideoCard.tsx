import React, { useEffect, useState } from "react";
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  TouchableWithoutFeedback,
  Dimensions,
  Platform,
  Animated,
  Easing,
  PanResponder,
} from "react-native";
import { useVideoPlayer, VideoView } from "expo-video";
import { FontAwesome } from "@expo/vector-icons";

const { width, height } = Dimensions.get("window");

// Target portrait 1080p aspect (1080x1920 -> 9:16)
const TARGET_WIDTH = 1080;
const TARGET_HEIGHT = 1920;
const VIDEO_ASPECT = TARGET_HEIGHT / TARGET_WIDTH; // 16/9 ~ 1.7777

// Compute display size that fits the device window while preserving 9:16 portrait aspect
let VIDEO_DISPLAY_WIDTH = width;
let VIDEO_DISPLAY_HEIGHT = Math.round(VIDEO_DISPLAY_WIDTH * VIDEO_ASPECT);
// If the computed height exceeds available height, scale down to fit height instead
if (VIDEO_DISPLAY_HEIGHT > height) {
  VIDEO_DISPLAY_HEIGHT = height;
  VIDEO_DISPLAY_WIDTH = Math.round(VIDEO_DISPLAY_HEIGHT / VIDEO_ASPECT);
}
const VIDEO_LEFT_OFFSET = Math.round((width - VIDEO_DISPLAY_WIDTH) / 2);
const VIDEO_TOP_OFFSET = Math.round((height - VIDEO_DISPLAY_HEIGHT) / 2);

interface VideoCardProps {
  videoUri: string;
  title: string;
  likes: string;
  comments: number;
  shares: number;
  isActive: boolean;
}

export default function VideoCard({
  videoUri,
  title,
  likes,
  comments,
  shares,
  isActive,
}: VideoCardProps) {
  const player = useVideoPlayer(videoUri, (player) => {
    player.loop = true;
    // Mute on web to allow autoplay (browser autoplay policies usually require muted autoplay)
    // Keep sound on native platforms
    player.muted = Platform.OS === "web" ? true : false;

    // Basic status logging to help debug loading/playback issues
    // Use `any` to avoid TypeScript errors for runtime listener API
    (player as any).addListener?.("statusChange", (payload: any) => {
      console.log(
        "[VideoCard] player statusChange:",
        payload.status,
        payload.error
      );
    });
  });

  const [playerError, setPlayerError] = useState<string | null>(null);

  useEffect(() => {
    if (!player) return;

    console.log(
      "[VideoCard] isActive",
      isActive,
      "status",
      player.status,
      "playing",
      player.playing
    );

    if (isActive) {
      // If player is ready, start immediately
      if (player.status === "readyToPlay") {
        setPlayerError(null);
        player.play();
        return;
      }

      // Otherwise, wait for readyToPlay and play then. Capture and display errors.
      const onStatus = (payload: any) => {
        if (payload.status === "readyToPlay") {
          setPlayerError(null);
          player.play();
        }
        if (payload.status === "error") {
          const msg = payload.error?.message ?? "Unknown playback error";
          console.error("[VideoCard] player error:", payload.error);
          setPlayerError(msg);
        }
      };

      (player as any).addListener?.("statusChange", onStatus);

      return () => {
        (player as any).removeListener?.("statusChange", onStatus);
      };
    } else {
      player.pause();
    }
  }, [isActive, player, videoUri]);

  // Rotation animation for profile image
  const rotation = React.useRef(new Animated.Value(0)).current;
  const rotateInterpolation = rotation.interpolate({
    inputRange: [0, 1],
    outputRange: ["0deg", "360deg"],
  });

  React.useEffect(() => {
    const anim = Animated.loop(
      Animated.timing(rotation, {
        toValue: 1,
        duration: 10000, // 10s for a slow rotation
        easing: Easing.linear,
        useNativeDriver: true,
      })
    );
    anim.start();
    return () => anim.stop();
  }, [rotation]);

  // Volume control state
  const [volume, setVolume] = React.useState<number>(1);
  const [muted, setMuted] = React.useState<boolean>(false);
  const prevVolume = React.useRef<number>(volume);

  const applyVolumeToPlayer = React.useCallback(
    (v: number, m: boolean) => {
      if (!player) return;
      try {
        // Prefer explicit API when available
        (player as any).setVolume?.(v);
        (player as any).setIsMuted?.(m);
        // Fallback to direct property set
        (player as any).volume = v;
        (player as any).muted = m;
      } catch (e) {
        // Ignore - best effort
        console.warn("[VideoCard] volume set failed:", e);
      }
    },
    [player]
  );

  React.useEffect(() => {
    applyVolumeToPlayer(volume, muted);
  }, [volume, muted, applyVolumeToPlayer]);

  const toggleMute = () => {
    if (!muted) {
      prevVolume.current = volume;
      setMuted(true);
      setVolume(0);
    } else {
      setMuted(false);
      const restore = prevVolume.current || 1;
      setVolume(restore);
    }
  };

  // Slider popup & pan responder with animated reveal + auto-hide
  const [sliderVisible, setSliderVisible] = React.useState<boolean>(false);

  // reveal animation (0 = closed, 1 = open)
  const revealAnim = React.useRef(new Animated.Value(0)).current;
  const SLIDER_FULL_WIDTH = 140;
  const sliderWidthAnim = revealAnim.interpolate({
    inputRange: [0, 1],
    outputRange: [0, SLIDER_FULL_WIDTH],
  });
  const iconOpacity = revealAnim.interpolate({
    inputRange: [0, 1],
    outputRange: [1, 0],
  });

  // Auto-hide timers
  const AUTO_HIDE_MS = 3000;
  const ICON_HIDE_MS = 3000;
  const autoHideTimer = React.useRef<ReturnType<typeof setTimeout> | null>(
    null
  );
  const iconHideTimer = React.useRef<ReturnType<typeof setTimeout> | null>(
    null
  );

  // icon hidden state (after inactivity)
  const [iconHidden, setIconHidden] = React.useState<boolean>(false);

  const clearAutoHide = () => {
    if (autoHideTimer.current) {
      clearTimeout(autoHideTimer.current as any);
      autoHideTimer.current = null;
    }
  };
  const startAutoHide = () => {
    clearAutoHide();
    autoHideTimer.current = setTimeout(() => {
      // animate close when timer fires
      closeSlider();
      autoHideTimer.current = null;
    }, AUTO_HIDE_MS);
  };
  const resetAutoHide = () => {
    if (sliderVisible) startAutoHide();
  };

  const clearIconAutoHide = () => {
    if (iconHideTimer.current) {
      clearTimeout(iconHideTimer.current as any);
      iconHideTimer.current = null;
    }
  };
  const startIconAutoHide = () => {
    clearIconAutoHide();
    iconHideTimer.current = setTimeout(() => {
      setIconHidden(true);
      iconHideTimer.current = null;
    }, ICON_HIDE_MS);
  };

  React.useEffect(() => {
    return () => {
      clearAutoHide();
      clearIconAutoHide();
    };
  }, []);

  // Reveal icon on any tap without blocking children
  const handleAnyTap = () => {
    if (iconHidden) {
      setIconHidden(false);
      clearIconAutoHide();
      startIconAutoHide();
    }
  };

  const knobAnim = React.useRef(new Animated.Value(volume)).current;
  React.useEffect(() => {
    Animated.timing(knobAnim, {
      toValue: volume,
      duration: 120,
      useNativeDriver: false, // width/translateX are not compatible with native driver here
    }).start();
  }, [volume, knobAnim]);

  const trackWidth = React.useRef<number>(120);
  const KNOB_SIZE = 16;

  const panResponder = React.useRef(
    PanResponder.create({
      onStartShouldSetPanResponder: () => true,
      onPanResponderGrant: () => {
        resetAutoHide();
        clearIconAutoHide();
      },
      onPanResponderMove: (e, g) => {
        resetAutoHide();
        clearIconAutoHide();
        const x = e.nativeEvent.locationX;
        const w = trackWidth.current || 120;
        const val = Math.min(1, Math.max(0, x / w));
        if (muted && val > 0) setMuted(false);
        prevVolume.current = val;
        setVolume(val);
      },
      onPanResponderRelease: () => resetAutoHide(),
    })
  ).current;

  const openSlider = () => {
    clearIconAutoHide();
    setIconHidden(false);
    setSliderVisible(true);
    Animated.timing(revealAnim, {
      toValue: 1,
      duration: 200,
      easing: Easing.out(Easing.cubic),
      useNativeDriver: false,
    }).start(() => {
      startAutoHide();
    });
  };

  const closeSlider = () => {
    clearAutoHide();
    Animated.timing(revealAnim, {
      toValue: 0,
      duration: 200,
      easing: Easing.in(Easing.cubic),
      useNativeDriver: false,
    }).start(() => {
      setSliderVisible(false);
      setIconHidden(false);
      startIconAutoHide();
    });
  };

  return (
    <TouchableWithoutFeedback onPress={handleAnyTap} accessible={false}>
      <View style={styles.container}>
        <VideoView
          style={{
            width: VIDEO_DISPLAY_WIDTH,
            height: VIDEO_DISPLAY_HEIGHT,
            position: "absolute",
            top: VIDEO_TOP_OFFSET,
            left: VIDEO_LEFT_OFFSET,
          }}
          player={player}
          fullscreenOptions={{ enable: false }}
          allowsPictureInPicture={false}
        />

        <View style={styles.overlay}>
          <Text style={styles.title}>{title}</Text>
          <Text style={styles.meta}>by TravelGirl · 1h</Text>

          <View style={styles.musicRow}>
            <FontAwesome name="music" size={16} color="#fff" />
            <Text style={styles.musicText}> Cool Vibes · Best Hits</Text>
          </View>

          {playerError ? (
            <View style={styles.errorContainer}>
              <Text style={styles.errorText}>{playerError}</Text>
              <TouchableOpacity
                style={styles.retryButton}
                onPress={async () => {
                  setPlayerError(null);
                  try {
                    await player.replaceAsync?.(videoUri);
                    if (player.status === "readyToPlay") player.play();
                  } catch (err) {
                    console.error("[VideoCard] retry error:", err);
                  }
                }}
              >
                <Text style={styles.retryText}>Retry</Text>
              </TouchableOpacity>
            </View>
          ) : null}

          <View style={styles.actions}>
            <TouchableOpacity style={[styles.icon, styles.iconPrimary]}>
              <FontAwesome name="heart" size={28} color="#FF2D55" />
              <Text style={styles.count}>{likes}</Text>
            </TouchableOpacity>

            <TouchableOpacity style={[styles.icon, styles.iconCircle]}>
              <FontAwesome name="comments" size={28} color="#fff" />
              <Text style={styles.count}>{comments}</Text>
            </TouchableOpacity>

            <TouchableOpacity style={[styles.icon, styles.iconCircle]}>
              <FontAwesome name="share" size={28} color="#fff" />
              <Text style={styles.count}>{shares}</Text>
            </TouchableOpacity>

            <TouchableOpacity style={styles.profileCircle} activeOpacity={0.9}>
              <Animated.Image
                source={require("../assets/images/icon.png")}
                style={[
                  styles.profileImage,
                  { transform: [{ rotate: rotateInterpolation }] },
                ]}
              />
            </TouchableOpacity>
          </View>
        </View>

        {sliderVisible ? (
          <TouchableWithoutFeedback onPress={() => closeSlider()}>
            <View style={styles.outsideOverlay} />
          </TouchableWithoutFeedback>
        ) : null}

        <View
          style={[styles.volumeControl, { zIndex: 9999, elevation: 20 }]}
          pointerEvents="box-none"
        >
          {!iconHidden ? (
            <Animated.View
              style={{ opacity: iconOpacity }}
              pointerEvents={sliderVisible ? "none" : "auto"}
            >
              <TouchableOpacity
                onPress={() => (sliderVisible ? closeSlider() : openSlider())}
                onLongPress={toggleMute}
                style={styles.volIconBtn}
                accessibilityLabel="Open volume slider (tap) / Mute (long press)"
                hitSlop={{ top: 8, bottom: 8, left: 8, right: 8 }}
              >
                <FontAwesome
                  name={muted ? "volume-off" : "volume-up"}
                  size={18}
                  color="#fff"
                />
              </TouchableOpacity>
            </Animated.View>
          ) : (
            <TouchableOpacity
              style={styles.invisibleTouchArea}
              onPress={() => {
                setIconHidden(false);
                openSlider();
              }}
              accessibilityLabel="Show volume control"
            />
          )}

          {!iconHidden && !sliderVisible ? (
            <Text style={styles.volumeText}>
              {muted ? "Muted" : `${Math.round(volume * 100)}%`}
            </Text>
          ) : null}

          {/* Animated reveal for the slider: it grows to the right of the icon */}
          {sliderVisible ? (
            <Animated.View
              style={[styles.sliderPop, { width: sliderWidthAnim }]}
              pointerEvents="box-none"
            >
              <View
                style={styles.sliderTrack}
                onLayout={(e) => {
                  trackWidth.current = e.nativeEvent.layout.width;
                }}
                {...panResponder.panHandlers}
              >
                <Animated.View
                  style={[
                    styles.sliderFill,
                    {
                      width: knobAnim.interpolate({
                        inputRange: [0, 1],
                        outputRange: [0, trackWidth.current || 120],
                      }),
                    },
                  ]}
                />
                <Animated.View
                  style={[
                    styles.sliderKnob,
                    {
                      transform: [
                        {
                          translateX: knobAnim.interpolate({
                            inputRange: [0, 1],
                            outputRange: [
                              -(KNOB_SIZE / 2),
                              (trackWidth.current || 120) - KNOB_SIZE / 2,
                            ],
                          }),
                        },
                      ],
                      left: 0,
                    },
                  ]}
                  {...panResponder.panHandlers}
                />
              </View>

              <TouchableOpacity
                onPress={() => {
                  toggleMute();
                }}
                style={styles.popMuteBtn}
                accessibilityLabel="Toggle mute"
              >
                <FontAwesome
                  name={muted ? "volume-off" : "volume-up"}
                  size={14}
                  color="#fff"
                />
              </TouchableOpacity>
            </Animated.View>
          ) : null}
        </View>
      </View>
    </TouchableWithoutFeedback>
  );
}

const styles = StyleSheet.create({
  container: { width, height, backgroundColor: "#000", overflow: "hidden" },
  video: { width, height },
  overlay: {
    position: "absolute",
    bottom: 50,
    left: 20,
    right: 20,
    paddingRight: 20,
    borderColor: "#f00", // Debugging border
    borderWidth: 0,
  },
  title: { color: "#fff", fontSize: 22, fontWeight: "bold", marginBottom: 6 },
  meta: { color: "#ddd", fontSize: 14, marginBottom: 10 },
  musicRow: { flexDirection: "row", alignItems: "center", marginBottom: 12 },
  musicText: { color: "#fff", fontSize: 14 },
  actions: {
    position: "absolute",
    right: -20,
    bottom: 100,
    justifyContent: "space-between",
    height: 140,
    alignItems: "center",
    width: 60,
    borderColor: "rgba(21, 255, 0, 1)", // Debugging border
    borderWidth: 0,
  },
  icon: { alignItems: "center", marginBottom: 8 },
  iconPrimary: {
    width: 56,
    height: 56,
    alignItems: "center",
    justifyContent: "center",
    borderColor: "rgba(21, 255, 0, 1)", // Debugging border
    borderWidth: 0,
  },
  iconCircle: {
    width: 48,
    height: 48,
    borderRadius: 24,
    backgroundColor: "rgba(21, 255, 0, 0)",
    alignItems: "center",
    justifyContent: "center",
  },
  count: { color: "#fff", fontSize: 14, marginTop: 4 },
  profileCircle: {
    width: 44,
    height: 44,
    borderRadius: 22,
    overflow: "hidden",
    borderWidth: 0,
    borderColor: "#ee1e1eff",
    marginTop: 4,
  },
  profileImage: { width: "100%", height: "100%" },
  volumeControl: {
    position: "absolute",
    top: Platform.OS === "ios" ? 44 : 20,
    left: 20,
    flexDirection: "row",
    alignItems: "center",
    backgroundColor: "rgba(0,0,0,0.45)",
    paddingHorizontal: 8,
    paddingVertical: 6,
    borderRadius: 20,
    zIndex: 30,
  },
  volIconBtn: {
    width: 36,
    height: 36,
    borderRadius: 18,
    alignItems: "center",
    justifyContent: "center",
    marginRight: 8,
    backgroundColor: "rgba(255,255,255,0.03)",
  },
  volumeText: {
    color: "#fff",
    fontSize: 12,
    minWidth: 36,
    textAlign: "center",
  },
  sliderPop: {
    position: "absolute",
    left: 48,
    top: -8,
    backgroundColor: "rgba(0,0,0,0.6)",
    padding: 8,
    borderRadius: 10,
    flexDirection: "row",
    alignItems: "center",
    zIndex: 10000,
  },
  outsideOverlay: {
    ...StyleSheet.absoluteFillObject,
    zIndex: 9000,
    backgroundColor: "transparent",
  },
  // Make sure the top-level TouchableWithoutFeedback does not obscure children
  containerTouchable: {
    flex: 1,
  },
  sliderTrack: {
    flex: 1,
    height: 6,
    backgroundColor: "rgba(255,255,255,0.12)",
    borderRadius: 4,
    overflow: "hidden",
  },
  invisibleTouchArea: {
    width: 36,
    height: 36,
    borderRadius: 18,
    marginRight: 8,
  },
  sliderFill: {
    position: "absolute",
    left: 0,
    top: 0,
    bottom: 0,
    backgroundColor: "#fff",
    opacity: 0.95,
    width: 0,
  },
  sliderKnob: {
    position: "absolute",
    top: -5,
    width: 16,
    height: 16,
    borderRadius: 8,
    backgroundColor: "#fff",
    borderWidth: 1,
    borderColor: "rgba(0,0,0,0.5)",
  },
  popMuteBtn: {
    marginLeft: 8,
    padding: 6,
    borderRadius: 12,
    backgroundColor: "rgba(255,255,255,0.04)",
  },
  errorContainer: {
    backgroundColor: "rgba(0,0,0,0.6)",
    padding: 8,
    borderRadius: 8,
    marginBottom: 12,
  },
  errorText: { color: "#ffcccb", fontSize: 14, marginBottom: 8 },
  retryButton: {
    backgroundColor: "#fff",
    paddingVertical: 6,
    paddingHorizontal: 12,
    borderRadius: 6,
  },
  retryText: { color: "#000", fontWeight: "600" },
});
