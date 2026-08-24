#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <span>
#include <type_traits>
#include <vector>

namespace baka {

namespace {
    template<size_t SZ>
    struct alignas(std::max_align_t) ArenaChunk {
        std::array<std::byte, SZ> data;
    };

    // need for calling dtors
    // type erasure container
    class TEActivationRecord {
        public:
            virtual void cleanup() = 0;
    };

    // linked list of activation records
    struct ActivationRecordNode {
        ActivationRecordNode* Next;
        TEActivationRecord* ActivationRecord;

        static ActivationRecordNode dummy() {
            return ActivationRecordNode{
                .Next = nullptr,
                .ActivationRecord = nullptr,
            };
        }
    };

    template<class T>
    class TypedActivationRecord_ : public TEActivationRecord {
        T* ptr_;

        public:
        TypedActivationRecord_(T* ptr) : ptr_(ptr) {};

        virtual void cleanup() override {
            std::destroy_at<T>(ptr_);
        }
    };

    template<class T>
    class TypedArrayActivationRecord_ : public TEActivationRecord {
        T* _ptr;
        size_t n;

        public:
        TypedArrayActivationRecord_(T* ptr, size_t n) : _ptr(ptr), n(n) {};

        virtual void cleanup() override {
            for(auto i = 0uz; i < n ; i++) {
                std::destroy_at<T>(_ptr + i);
            }
        }
    };
}

namespace base {

    template<size_t SZ = 1024>
    class Arena {
        std::vector<std::unique_ptr<ArenaChunk<SZ>>> InUseChunks;
        ArenaChunk<SZ>* ActiveChunk;
        size_t UsedBytes;
        std::unique_ptr<ActivationRecordNode> ActivationRecordHead; // dummy head node is raii
        ActivationRecordNode* ActivationRecordTail;


        void NewChunkAlloc() {
            InUseChunks.emplace_back(std::make_unique<ArenaChunk<SZ>>());
            ActiveChunk = InUseChunks.back().get();
            UsedBytes = 0;
        }

        public:

            Arena() : ActivationRecordHead(std::make_unique<ActivationRecordNode>(ActivationRecordNode::dummy())) {
                NewChunkAlloc();
                ActivationRecordTail = ActivationRecordHead.get();
            }

            ~Arena() {
                // call cleanup on all activation records
                ActivationRecordNode* curr = ActivationRecordHead.get();
                while(curr != nullptr) {
                    if(curr->ActivationRecord == nullptr) {
                        curr = curr->Next;
                        continue;
                    }

                    curr->ActivationRecord->cleanup();

                    ActivationRecordNode* prev = curr;
                    curr = curr->Next;
                }
            }

            // dont need this stuff
            Arena(const Arena& other) = delete;
            Arena(Arena&& other) = delete;
            Arena& operator=(const Arena& other) = delete;
            Arena& operator=(Arena&& other) = delete;


            template<class T, class ...Args>
            requires (sizeof(T) <= SZ)
            std::span<T> AllocArray(size_t N ,Args&& ...args) {
                if(N * sizeof(T) > SZ) {
                    throw std::bad_alloc();
                }

                ActivationRecordNode* NewActivationRecordLLNode = nullptr;
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    // if not trivially destructible then push activation record

                    NewActivationRecordLLNode = Alloc<ActivationRecordNode>(nullptr, nullptr);
                    // assingn the activation record later

                    ActivationRecordTail->Next = NewActivationRecordLLNode;
                    ActivationRecordTail = NewActivationRecordLLNode;
                }

                int ct = 1;
                do {
                    size_t NearestAlignIdx = UsedBytes + (alignof(T) - (UsedBytes % alignof(T))) % alignof(T); // ciel div
                    size_t PossibleNewIdx = NearestAlignIdx + (sizeof(T) * N);

                    if(PossibleNewIdx <= SZ) {
                        std::byte* base = &ActiveChunk->data[0];
                        T* startPtr = reinterpret_cast<T*>(base + NearestAlignIdx);

                        for(auto i = 0; i < N; i++) {
                            std::construct_at<T>(startPtr + i, std::forward<Args>(args)...);
                        }

                        UsedBytes = PossibleNewIdx;

                        if constexpr (!std::is_trivially_destructible_v<T>) {
                            TEActivationRecord* NewTEActivationRecord = Alloc<TypedArrayActivationRecord_<T>>(startPtr, N);
                            assert(NewActivationRecordLLNode != nullptr);

                            NewActivationRecordLLNode->ActivationRecord = NewTEActivationRecord;
                        }


                        std::span<T> result(startPtr, N);
                        return result;
                    } else {
                        NewChunkAlloc();
                    }
                } while(ct--);

                throw std::bad_alloc();
            }

            template<class T, class ...Args>
            requires (sizeof(T) <= SZ)
            T* Alloc(Args&&... args) {

                // if constexpr alloc activation record
                ActivationRecordNode* NewActivationRecordLLNode = nullptr;
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    // if not trivially destructible then push activation record

                    NewActivationRecordLLNode = Alloc<ActivationRecordNode>(nullptr, nullptr);
                    // assingn the activation record later

                    ActivationRecordTail->Next = NewActivationRecordLLNode;
                    ActivationRecordTail = NewActivationRecordLLNode;
                }

                int ct = 1;
                do {
                    size_t NearestAlignIdx = UsedBytes + (alignof(T) - (UsedBytes % alignof(T))) % alignof(T); // ciel div
                    size_t PossibleNewIdx = NearestAlignIdx + sizeof(T);

                    if(PossibleNewIdx <= SZ) {
                        std::byte* base = &ActiveChunk->data[0];
                        T* retPtr = reinterpret_cast<T*>(base + NearestAlignIdx);

                        std::construct_at<T>(retPtr, std::forward<Args>(args)...);
                        UsedBytes = PossibleNewIdx;


                        if constexpr (!std::is_trivially_destructible_v<T>) {
                            TEActivationRecord* NewTEActivationRecord = Alloc<TypedActivationRecord_<T>>(retPtr);
                            assert(NewActivationRecordLLNode != nullptr);

                            NewActivationRecordLLNode->ActivationRecord = NewTEActivationRecord;
                        }

                        return retPtr;
                    } else {
                        NewChunkAlloc();
                    }
                } while(ct--);

                throw std::bad_alloc();
            }
    };
}
}
