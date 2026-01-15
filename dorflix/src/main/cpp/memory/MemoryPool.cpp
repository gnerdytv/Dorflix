#include "MemoryPool.h"
#include <cstring>
#include <algorithm>
#include <ctime>

MemoryPool::MemoryPool(size_t poolSize) 
    : m_poolSize(poolSize), m_usedSize(0), m_alignment(16), m_head(nullptr), m_allocationCount(0), m_peakUsage(0), m_totalAllocations(0), m_totalDeallocations(0) {
    m_pool = std::make_unique<uint8_t[]>(m_poolSize);
    initializePool();
}

MemoryPool::~MemoryPool() { 
    clear(); 
}

void* MemoryPool::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (size == 0) return nullptr;
    
    // Align size to specified alignment
    size_t alignedSize = (size + m_alignment - 1) & ~(m_alignment - 1);
    
    // Check if we have enough memory
    if (alignedSize > (m_poolSize - m_usedSize)) {
        return nullptr; // Not enough memory
    }
    
    MemoryBlock* block = findBestFit(alignedSize);
    if (!block) {
        return nullptr; // No suitable block found
    }
    
    splitBlock(block, alignedSize);
    block->isFree = false;
    
    m_usedSize += alignedSize;
    m_allocationCount++;
    m_totalAllocations++;
    
    if (m_usedSize > m_peakUsage) {
        m_peakUsage = m_usedSize;
    }
    
    return getBlockData(block);
}

void MemoryPool::deallocate(void* ptr) {
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    MemoryBlock* block = getBlockFromData(ptr);
    if (!block || block->isFree) {
        return; // Invalid pointer or already freed
    }
    
    block->isFree = true;
    m_usedSize -= block->size;
    m_allocationCount--;
    m_totalDeallocations++;
    
    mergeFreeBlocks();
}

void MemoryPool::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Mark all blocks as free except the head
    MemoryBlock* current = m_head;
    while (current) {
        if (current != m_head) {
            current->isFree = true;
        }
        current = current->next;
    }
    
    m_usedSize = 0;
    m_allocationCount = 0;
}

void MemoryPool::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_pool.reset();
    m_poolSize = 0;
    m_usedSize = 0;
    m_head = nullptr;
    m_allocationCount = 0;
    m_peakUsage = 0;
    m_totalAllocations = 0;
    m_totalDeallocations = 0;
}

size_t MemoryPool::getPoolSize() const { 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_poolSize; 
}

size_t MemoryPool::getUsedSize() const { 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_usedSize; 
}

size_t MemoryPool::getFreeSize() const { 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_poolSize - m_usedSize; 
}

size_t MemoryPool::getAllocationCount() const { 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_allocationCount; 
}

double MemoryPool::getUtilization() const { 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_poolSize > 0 ? static_cast<double>(m_usedSize) / m_poolSize : 0.0; 
}

void MemoryPool::setAlignment(size_t alignment) { 
    std::lock_guard<std::mutex> lock(m_mutex);
    m_alignment = alignment; 
}

void MemoryPool::initializePool() {
    if (!m_pool || m_poolSize == 0) return;
    
    // Create a single large free block that covers the entire pool
    m_head = reinterpret_cast<MemoryBlock*>(m_pool.get());
    m_head->size = m_poolSize - sizeof(MemoryBlock);
    m_head->isFree = true;
    m_head->next = nullptr;
    m_head->prev = nullptr;
}

MemoryPool::MemoryBlock* MemoryPool::findBestFit(size_t size) {
    MemoryBlock* bestFit = nullptr;
    MemoryBlock* current = m_head;
    
    while (current) {
        if (current->isFree && current->size >= size) {
            if (!bestFit || current->size < bestFit->size) {
                bestFit = current;
            }
        }
        current = current->next;
    }
    
    return bestFit;
}

void MemoryPool::splitBlock(MemoryBlock* block, size_t size) {
    if (!block || block->size < size + sizeof(MemoryBlock)) {
        return; // Cannot split further
    }
    
    size_t remainingSize = block->size - size - sizeof(MemoryBlock);
    
    // Create new block for remaining space
    MemoryBlock* newBlock = reinterpret_cast<MemoryBlock*>(
        reinterpret_cast<uint8_t*>(block) + sizeof(MemoryBlock) + size
    );
    newBlock->size = remainingSize;
    newBlock->isFree = true;
    newBlock->next = block->next;
    newBlock->prev = block;
    
    // Update links
    if (block->next) {
        block->next->prev = newBlock;
    }
    block->next = newBlock;
    
    // Update current block size
    block->size = size;
}

void MemoryPool::mergeFreeBlocks() {
    MemoryBlock* current = m_head;
    
    while (current && current->next) {
        if (current->isFree && current->next->isFree) {
            // Merge current block with next block
            current->size += sizeof(MemoryBlock) + current->next->size;
            current->next = current->next->next;
            
            if (current->next) {
                current->next->prev = current;
            }
        } else {
            current = current->next;
        }
    }
}

void* MemoryPool::getBlockData(MemoryBlock* block) {
    if (!block) return nullptr;
    return reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(block) + sizeof(MemoryBlock));
}

MemoryPool::MemoryBlock* MemoryPool::getBlockFromData(void* data) {
    if (!data) return nullptr;
    
    uint8_t* dataPtr = reinterpret_cast<uint8_t*>(data);
    MemoryBlock* block = reinterpret_cast<MemoryBlock*>(dataPtr - sizeof(MemoryBlock));
    
    // Validate that the block is within our pool
    if (dataPtr >= m_pool.get() && 
        dataPtr < m_pool.get() + m_poolSize &&
        block >= reinterpret_cast<MemoryBlock*>(m_pool.get()) &&
        block < reinterpret_cast<MemoryBlock*>(m_pool.get() + m_poolSize)) {
        return block;
    }
    
    return nullptr;
}
