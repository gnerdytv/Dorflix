package com.dorflix.app;

@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000\f\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0002\b\u0004\u0018\u0000 \u00042\u00020\u0001:\u0001\u0004B\t\b\u0002\u00a2\u0006\u0004\b\u0002\u0010\u0003\u00a8\u0006\u0005"}, d2 = {"Lcom/dorflix/app/CodecEnumerator;", "", "<init>", "()V", "Companion", "DorflixNative_debug"})
public final class CodecEnumerator {
    @org.jetbrains.annotations.NotNull()
    private static final java.util.Set<java.lang.String> codecBlacklist = null;
    @org.jetbrains.annotations.NotNull()
    private static final java.util.List<com.dorflix.app.CodecPriority> codecPriorityList = null;
    @org.jetbrains.annotations.NotNull()
    private static final java.lang.String TAG = "CodecEnumerator";
    @org.jetbrains.annotations.NotNull()
    public static final com.dorflix.app.CodecEnumerator.Companion Companion = null;
    
    private CodecEnumerator() {
        super();
    }
    
    @kotlin.jvm.JvmStatic()
    public static final void onCodecEnumerated(@org.jetbrains.annotations.NotNull()
    java.lang.String name, @org.jetbrains.annotations.NotNull()
    java.lang.String mimeType, boolean isEncoder, boolean isHardware, int maxWidth, int maxHeight, int profile, int level) {
    }
    
    @kotlin.jvm.JvmStatic()
    public static final void onEnumerationComplete(int totalCodecs) {
    }
    
    @kotlin.jvm.JvmStatic()
    public static final native void storeCodecInfo(@org.jetbrains.annotations.NotNull()
    java.lang.String name, @org.jetbrains.annotations.NotNull()
    java.lang.String mimeType, boolean isEncoder, boolean isHardware, int maxWidth, int maxHeight, int profile, int level, @org.jetbrains.annotations.NotNull()
    java.lang.String[] hdrSupport, @org.jetbrains.annotations.NotNull()
    java.lang.String[] colorFormats, int maxBitrate) {
    }
    
    @kotlin.jvm.JvmStatic()
    public static final native void signalEnumerationComplete(int totalCodecs) {
    }
    
    /**
     * Safely enumerate all codecs with error recovery
     * Returns true if enumeration succeeded, false if any errors occurred
     */
    @kotlin.jvm.JvmStatic()
    public static final boolean safeEnumerateAllCodecs() {
        return false;
    }
    
    /**
     * Enumerate all available MediaCodec instances with comprehensive logging
     */
    @kotlin.jvm.JvmStatic()
    public static final void enumerateAllCodecs() {
    }
    
    /**
     * Get detailed information about a specific codec
     */
    @kotlin.jvm.JvmStatic()
    public static final void getCodecDetails(@org.jetbrains.annotations.NotNull()
    java.lang.String codecName) {
    }
    
    /**
     * Enumerate all available MediaCodec instances with selective validation
     * @param validate Whether to perform expensive full validation (default: true)
     * @param measurePerformance Whether to measure codec performance (default: false)
     * @return List of validated codec information
     */
    @kotlin.jvm.JvmStatic()
    @org.jetbrains.annotations.NotNull()
    public static final java.util.List<com.dorflix.app.CodecInfo> enumerateCodecs(boolean validate, boolean measurePerformance) {
        return null;
    }
    
    /**
     * Enhanced enumeration with caching support
     */
    @kotlin.jvm.JvmStatic()
    public static final void enumerateAllCodecsWithCache() {
    }
    
    /**
     * Intelligent codec selection based on video format requirements
     * Prioritizes hardware acceleration, then performance metrics
     */
    @kotlin.jvm.JvmStatic()
    @org.jetbrains.annotations.Nullable()
    public static final com.dorflix.app.CodecInfo selectOptimalCodec(@org.jetbrains.annotations.NotNull()
    java.lang.String mimeType, int width, int height, int bitrate, boolean requireHardware) {
        return null;
    }
    
    /**
     * Create prioritized fallback codec chains for hardware-only codecs
     */
    @kotlin.jvm.JvmStatic()
    @org.jetbrains.annotations.NotNull()
    public static final java.util.List<com.dorflix.app.CodecInfo> createHardwareFallbackChain(@org.jetbrains.annotations.NotNull()
    java.lang.String mimeType) {
        return null;
    }
    
    /**
     * Get codec selection recommendations for different video formats
     */
    @kotlin.jvm.JvmStatic()
    @org.jetbrains.annotations.NotNull()
    public static final java.util.Map<java.lang.String, java.util.List<com.dorflix.app.CodecInfo>> getCodecRecommendations() {
        return null;
    }
    
    /**
     * Check if modern codec support is available (AV1, HEVC HDR, VP9 HDR)
     */
    @kotlin.jvm.JvmStatic()
    @org.jetbrains.annotations.NotNull()
    public static final java.util.Map<java.lang.String, java.lang.Boolean> checkModernCodecSupport() {
        return null;
    }
    
    /**
     * Force cache refresh (useful when device capabilities change)
     */
    @kotlin.jvm.JvmStatic()
    public static final void refreshCodecCache() {
    }
    
    @kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000x\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0002\b\u0003\n\u0002\u0010\"\n\u0002\u0010\u000e\n\u0000\n\u0002\u0010 \n\u0002\u0018\u0002\n\u0000\n\u0002\u0010\b\n\u0002\b\u0002\n\u0002\u0010\u000b\n\u0002\b\u0002\n\u0002\u0010\u0002\n\u0002\b\u000b\n\u0002\u0010\u0011\n\u0002\b\t\n\u0002\u0018\u0002\n\u0002\b\u0003\n\u0002\u0018\u0002\n\u0002\b\u0006\n\u0002\u0010\t\n\u0002\b\u0002\n\u0002\u0018\u0002\n\u0002\b\u0002\n\u0002\u0018\u0002\n\u0002\b\u0007\n\u0002\u0010!\n\u0002\b\b\n\u0002\u0010$\n\u0002\b\u0004\b\u0086\u0003\u0018\u00002\u00020\u0001B\t\b\u0002\u00a2\u0006\u0004\b\u0002\u0010\u0003J\u0010\u0010\n\u001a\u00020\u000b2\u0006\u0010\f\u001a\u00020\u0006H\u0002J\u0010\u0010\r\u001a\u00020\u000e2\u0006\u0010\u000f\u001a\u00020\u0006H\u0002JH\u0010\u0010\u001a\u00020\u00112\u0006\u0010\u000f\u001a\u00020\u00062\u0006\u0010\u0012\u001a\u00020\u00062\u0006\u0010\u0013\u001a\u00020\u000e2\u0006\u0010\u0014\u001a\u00020\u000e2\u0006\u0010\u0015\u001a\u00020\u000b2\u0006\u0010\u0016\u001a\u00020\u000b2\u0006\u0010\u0017\u001a\u00020\u000b2\u0006\u0010\u0018\u001a\u00020\u000bH\u0007J\u0010\u0010\u0019\u001a\u00020\u00112\u0006\u0010\u001a\u001a\u00020\u000bH\u0007Jm\u0010\u001b\u001a\u00020\u00112\u0006\u0010\u000f\u001a\u00020\u00062\u0006\u0010\u0012\u001a\u00020\u00062\u0006\u0010\u0013\u001a\u00020\u000e2\u0006\u0010\u0014\u001a\u00020\u000e2\u0006\u0010\u0015\u001a\u00020\u000b2\u0006\u0010\u0016\u001a\u00020\u000b2\u0006\u0010\u0017\u001a\u00020\u000b2\u0006\u0010\u0018\u001a\u00020\u000b2\f\u0010\u001c\u001a\b\u0012\u0004\u0012\u00020\u00060\u001d2\f\u0010\u001e\u001a\b\u0012\u0004\u0012\u00020\u00060\u001d2\u0006\u0010\u001f\u001a\u00020\u000bH\u0087 J\u0011\u0010 \u001a\u00020\u00112\u0006\u0010\u001a\u001a\u00020\u000bH\u0087 J\b\u0010\"\u001a\u00020\u000eH\u0007J\b\u0010#\u001a\u00020\u0011H\u0007J\u0010\u0010$\u001a\u00020\u00112\u0006\u0010\f\u001a\u00020\u0006H\u0007J\u0010\u0010%\u001a\u00020\u000e2\u0006\u0010&\u001a\u00020\'H\u0002J\u0010\u0010(\u001a\u00020\u000b2\u0006\u0010\f\u001a\u00020\u0006H\u0002J\u0012\u0010)\u001a\u00020\u000e2\b\u0010\f\u001a\u0004\u0018\u00010\u0006H\u0002J$\u0010*\u001a\u000e\u0012\u0004\u0012\u00020\u000b\u0012\u0004\u0012\u00020\u000b0+2\u0006\u0010&\u001a\u00020\'2\u0006\u0010\u0012\u001a\u00020\u0006H\u0002J\u001e\u0010,\u001a\b\u0012\u0004\u0012\u00020\u00060\b2\u0006\u0010&\u001a\u00020\'2\u0006\u0010\u0012\u001a\u00020\u0006H\u0002J\u001e\u0010-\u001a\b\u0012\u0004\u0012\u00020\u00060\b2\u0006\u0010&\u001a\u00020\'2\u0006\u0010\u0012\u001a\u00020\u0006H\u0002J\u0018\u0010.\u001a\u00020\u000b2\u0006\u0010&\u001a\u00020\'2\u0006\u0010\u0012\u001a\u00020\u0006H\u0002J\u0018\u0010/\u001a\u00020\u000e2\u0006\u0010&\u001a\u00020\'2\u0006\u0010\u0012\u001a\u00020\u0006H\u0002J\u0018\u00100\u001a\u00020\u000e2\u0006\u0010&\u001a\u00020\'2\u0006\u0010\u0012\u001a\u00020\u0006H\u0002J\u0018\u00101\u001a\u0002022\u0006\u0010&\u001a\u00020\'2\u0006\u0010\u0012\u001a\u00020\u0006H\u0002J\u001e\u00103\u001a\u00020\u00112\f\u00104\u001a\b\u0012\u0004\u0012\u0002050\b2\u0006\u00106\u001a\u000202H\u0002J\n\u00107\u001a\u0004\u0018\u000108H\u0002J\b\u00109\u001a\u00020\u0011H\u0002J\b\u0010:\u001a\u00020\u000eH\u0002J\"\u0010;\u001a\b\u0012\u0004\u0012\u0002050\b2\b\b\u0002\u0010<\u001a\u00020\u000e2\b\b\u0002\u0010=\u001a\u00020\u000eH\u0007J&\u0010>\u001a\u00020\u00112\f\u0010?\u001a\b\u0012\u0004\u0012\u0002050@2\u0006\u0010<\u001a\u00020\u000e2\u0006\u0010=\u001a\u00020\u000eH\u0002J\b\u0010A\u001a\u00020\u0011H\u0007J6\u0010B\u001a\u0004\u0018\u0001052\u0006\u0010\u0012\u001a\u00020\u00062\u0006\u0010C\u001a\u00020\u000b2\u0006\u0010D\u001a\u00020\u000b2\b\b\u0002\u0010E\u001a\u00020\u000b2\b\b\u0002\u0010F\u001a\u00020\u000eH\u0007J\u0016\u0010G\u001a\b\u0012\u0004\u0012\u0002050\b2\u0006\u0010\u0012\u001a\u00020\u0006H\u0007J\u001a\u0010H\u001a\u0014\u0012\u0004\u0012\u00020\u0006\u0012\n\u0012\b\u0012\u0004\u0012\u0002050\b0IH\u0007J\u0014\u0010J\u001a\u000e\u0012\u0004\u0012\u00020\u0006\u0012\u0004\u0012\u00020\u000e0IH\u0007J\b\u0010K\u001a\u00020\u0011H\u0007J\u0016\u0010L\u001a\u00020\u00112\f\u0010?\u001a\b\u0012\u0004\u0012\u0002050@H\u0002R\u0014\u0010\u0004\u001a\b\u0012\u0004\u0012\u00020\u00060\u0005X\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u0014\u0010\u0007\u001a\b\u0012\u0004\u0012\u00020\t0\bX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010!\u001a\u00020\u0006X\u0082T\u00a2\u0006\u0002\n\u0000\u00a8\u0006M"}, d2 = {"Lcom/dorflix/app/CodecEnumerator$Companion;", "", "<init>", "()V", "codecBlacklist", "", "", "codecPriorityList", "", "Lcom/dorflix/app/CodecPriority;", "calculateCodecPriority", "", "codecName", "shouldSkipCodec", "", "name", "onCodecEnumerated", "", "mimeType", "isEncoder", "isHardware", "maxWidth", "maxHeight", "profile", "level", "onEnumerationComplete", "totalCodecs", "storeCodecInfo", "hdrSupport", "", "colorFormats", "maxBitrate", "signalEnumerationComplete", "TAG", "safeEnumerateAllCodecs", "enumerateAllCodecs", "getCodecDetails", "getIsHardwareAccelerated", "codecInfo", "Landroid/media/MediaCodecInfo;", "extractH264ProfileLevel", "isHardwareAcceleratedFallback", "extractRealProfileLevel", "Lkotlin/Pair;", "extractHDRSupport", "extractColorFormats", "extractMaxBitrate", "validateCodecCapabilityLightweight", "validateCodecCapability", "measureCodecPerformance", "", "saveEnumerationResults", "codecs", "Lcom/dorflix/app/CodecInfo;", "enumerationTime", "loadEnumerationResults", "Lcom/dorflix/app/CodecEnumerationResult;", "clearEnumerationCache", "shouldRefreshEnumeration", "enumerateCodecs", "validate", "measurePerformance", "enumerateCodecsInternal", "collectedCodecs", "", "enumerateAllCodecsWithCache", "selectOptimalCodec", "width", "height", "bitrate", "requireHardware", "createHardwareFallbackChain", "getCodecRecommendations", "", "checkModernCodecSupport", "refreshCodecCache", "enumerateAndCollect", "DorflixNative_debug"})
    public static final class Companion {
        
        private Companion() {
            super();
        }
        
        /**
         * Calculate priority score for a codec based on its name
         * Higher scores indicate better/preferred codecs
         */
        private final int calculateCodecPriority(java.lang.String codecName) {
            return 0;
        }
        
        /**
         * Check if a codec should be skipped based on the blacklist
         */
        private final boolean shouldSkipCodec(java.lang.String name) {
            return false;
        }
        
        @kotlin.jvm.JvmStatic()
        public final void onCodecEnumerated(@org.jetbrains.annotations.NotNull()
        java.lang.String name, @org.jetbrains.annotations.NotNull()
        java.lang.String mimeType, boolean isEncoder, boolean isHardware, int maxWidth, int maxHeight, int profile, int level) {
        }
        
        @kotlin.jvm.JvmStatic()
        public final void onEnumerationComplete(int totalCodecs) {
        }
        
        @kotlin.jvm.JvmStatic()
        public final void storeCodecInfo(@org.jetbrains.annotations.NotNull()
        java.lang.String name, @org.jetbrains.annotations.NotNull()
        java.lang.String mimeType, boolean isEncoder, boolean isHardware, int maxWidth, int maxHeight, int profile, int level, @org.jetbrains.annotations.NotNull()
        java.lang.String[] hdrSupport, @org.jetbrains.annotations.NotNull()
        java.lang.String[] colorFormats, int maxBitrate) {
        }
        
        @kotlin.jvm.JvmStatic()
        public final void signalEnumerationComplete(int totalCodecs) {
        }
        
        /**
         * Safely enumerate all codecs with error recovery
         * Returns true if enumeration succeeded, false if any errors occurred
         */
        @kotlin.jvm.JvmStatic()
        public final boolean safeEnumerateAllCodecs() {
            return false;
        }
        
        /**
         * Enumerate all available MediaCodec instances with comprehensive logging
         */
        @kotlin.jvm.JvmStatic()
        public final void enumerateAllCodecs() {
        }
        
        /**
         * Get detailed information about a specific codec
         */
        @kotlin.jvm.JvmStatic()
        public final void getCodecDetails(@org.jetbrains.annotations.NotNull()
        java.lang.String codecName) {
        }
        
        /**
         * Safely determines if a codec is hardware accelerated, with backward compatibility
         * for Android API levels below 29 where isHardwareAccelerated() doesn't exist.
         */
        private final boolean getIsHardwareAccelerated(android.media.MediaCodecInfo codecInfo) {
            return false;
        }
        
        /**
         * Extract H.264 profile/level information from codec name
         * This is a heuristic approach since Android doesn't expose profile/level for decoders
         */
        private final int extractH264ProfileLevel(java.lang.String codecName) {
            return 0;
        }
        
        /**
         * Fallback method to determine hardware acceleration for API < 29
         * Based on common codec naming conventions:
         * - OMX.* : OpenMAX hardware codecs
         * - c2.* : Codec2.0 hardware codecs
         */
        private final boolean isHardwareAcceleratedFallback(java.lang.String codecName) {
            return false;
        }
        
        /**
         * Extract real profile/level support from MediaCodec capabilities
         * Instead of heuristic name matching, query actual capabilities
         */
        private final kotlin.Pair<java.lang.Integer, java.lang.Integer> extractRealProfileLevel(android.media.MediaCodecInfo codecInfo, java.lang.String mimeType) {
            return null;
        }
        
        /**
         * Extract HDR support information from codec capabilities
         */
        private final java.util.List<java.lang.String> extractHDRSupport(android.media.MediaCodecInfo codecInfo, java.lang.String mimeType) {
            return null;
        }
        
        /**
         * Extract color format information from codec capabilities
         */
        private final java.util.List<java.lang.String> extractColorFormats(android.media.MediaCodecInfo codecInfo, java.lang.String mimeType) {
            return null;
        }
        
        /**
         * Extract maximum bitrate information from codec capabilities
         */
        private final int extractMaxBitrate(android.media.MediaCodecInfo codecInfo, java.lang.String mimeType) {
            return 0;
        }
        
        /**
         * Lightweight codec capability validation without creating codec instances
         * Quick validation using only capabilities information
         */
        private final boolean validateCodecCapabilityLightweight(android.media.MediaCodecInfo codecInfo, java.lang.String mimeType) {
            return false;
        }
        
        /**
         * Validate codec capability by attempting to create and configure it
         * Tests basic codec functionality before marking as available
         */
        private final boolean validateCodecCapability(android.media.MediaCodecInfo codecInfo, java.lang.String mimeType) {
            return false;
        }
        
        /**
         * Get codec performance metrics by running a quick benchmark
         */
        private final long measureCodecPerformance(android.media.MediaCodecInfo codecInfo, java.lang.String mimeType) {
            return 0L;
        }
        
        /**
         * Save enumeration results to persistent storage
         */
        private final void saveEnumerationResults(java.util.List<com.dorflix.app.CodecInfo> codecs, long enumerationTime) {
        }
        
        /**
         * Load enumeration results from persistent storage
         */
        private final com.dorflix.app.CodecEnumerationResult loadEnumerationResults() {
            return null;
        }
        
        /**
         * Clear the enumeration cache
         */
        private final void clearEnumerationCache() {
        }
        
        /**
         * Check if enumeration results need to be refreshed
         * Enhanced cache invalidation with multiple triggers for robustness
         */
        private final boolean shouldRefreshEnumeration() {
            return false;
        }
        
        /**
         * Enumerate all available MediaCodec instances with selective validation
         * @param validate Whether to perform expensive full validation (default: true)
         * @param measurePerformance Whether to measure codec performance (default: false)
         * @return List of validated codec information
         */
        @kotlin.jvm.JvmStatic()
        @org.jetbrains.annotations.NotNull()
        public final java.util.List<com.dorflix.app.CodecInfo> enumerateCodecs(boolean validate, boolean measurePerformance) {
            return null;
        }
        
        /**
         * Internal enumeration function with conditional validation and performance measurement
         */
        private final void enumerateCodecsInternal(java.util.List<com.dorflix.app.CodecInfo> collectedCodecs, boolean validate, boolean measurePerformance) {
        }
        
        /**
         * Enhanced enumeration with caching support
         */
        @kotlin.jvm.JvmStatic()
        public final void enumerateAllCodecsWithCache() {
        }
        
        /**
         * Intelligent codec selection based on video format requirements
         * Prioritizes hardware acceleration, then performance metrics
         */
        @kotlin.jvm.JvmStatic()
        @org.jetbrains.annotations.Nullable()
        public final com.dorflix.app.CodecInfo selectOptimalCodec(@org.jetbrains.annotations.NotNull()
        java.lang.String mimeType, int width, int height, int bitrate, boolean requireHardware) {
            return null;
        }
        
        /**
         * Create prioritized fallback codec chains for hardware-only codecs
         */
        @kotlin.jvm.JvmStatic()
        @org.jetbrains.annotations.NotNull()
        public final java.util.List<com.dorflix.app.CodecInfo> createHardwareFallbackChain(@org.jetbrains.annotations.NotNull()
        java.lang.String mimeType) {
            return null;
        }
        
        /**
         * Get codec selection recommendations for different video formats
         */
        @kotlin.jvm.JvmStatic()
        @org.jetbrains.annotations.NotNull()
        public final java.util.Map<java.lang.String, java.util.List<com.dorflix.app.CodecInfo>> getCodecRecommendations() {
            return null;
        }
        
        /**
         * Check if modern codec support is available (AV1, HEVC HDR, VP9 HDR)
         */
        @kotlin.jvm.JvmStatic()
        @org.jetbrains.annotations.NotNull()
        public final java.util.Map<java.lang.String, java.lang.Boolean> checkModernCodecSupport() {
            return null;
        }
        
        /**
         * Force cache refresh (useful when device capabilities change)
         */
        @kotlin.jvm.JvmStatic()
        public final void refreshCodecCache() {
        }
        
        /**
         * Perform enumeration and collect results for caching
         */
        private final void enumerateAndCollect(java.util.List<com.dorflix.app.CodecInfo> collectedCodecs) {
        }
    }
}