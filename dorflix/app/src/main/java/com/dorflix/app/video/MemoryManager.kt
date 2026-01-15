package com.dorflix.app.video

/**
 * Memory manager for efficient memory allocation and deallocation
 */
class MemoryManager(private val poolSize: Int) {
    
    companion object {
        // Load the native library
        init {
            System.loadLibrary("dorflix-native")
        }
    }
    
    // Native pointer to C++ MemoryPool instance
    private var nativePoolPtr: Long = 0
    
    init {
        // Create native memory pool instance
        nativePoolPtr = nativeCreateMemoryPool(poolSize)
    }
    
    /**
     * Allocate memory from the pool
     * @param size Size of memory to allocate in bytes
     * @return Pointer to allocated memory, or null if allocation fails
     */
    fun allocate(size: Int): Long {
        return if (nativePoolPtr != 0L) {
            nativeAllocate(nativePoolPtr, size)
        } else 0L
    }
    
    /**
     * Free allocated memory
     * @param ptr Pointer to memory to free
     */
    fun deallocate(ptr: Long) {
        if (nativePoolPtr != 0L && ptr != 0L) {
            nativeFree(nativePoolPtr, ptr)
        }
    }
    
    /**
     * Get the total size of the memory pool
     * @return Pool size in bytes
     */
    fun getPoolSize(): Int {
        return if (nativePoolPtr != 0L) {
            nativeGetPoolSize(nativePoolPtr)
        } else 0
    }
    
    /**
     * Get the amount of memory currently used
     * @return Used memory size in bytes
     */
    fun getUsedSize(): Int {
        return if (nativePoolPtr != 0L) {
            nativeGetUsedSize(nativePoolPtr)
        } else 0
    }
    
    /**
     * Release resources and clean up
     */
    fun release() {
        if (nativePoolPtr != 0L) {
            nativeDestroyMemoryPool(nativePoolPtr)
            nativePoolPtr = 0
        }
    }
    
    // Native method declarations
    private external fun nativeCreateMemoryPool(poolSize: Int): Long
    private external fun nativeDestroyMemoryPool(poolPtr: Long)
    private external fun nativeAllocate(poolPtr: Long, size: Int): Long
    private external fun nativeFree(poolPtr: Long, ptr: Long)
    private external fun nativeGetPoolSize(poolPtr: Long): Int
    private external fun nativeGetUsedSize(poolPtr: Long): Int
}
