#include "win_sound.hpp"

#include <dsound.h>

#include <cstdio>

static_assert(sizeof(DWORD) == sizeof(uint32));

namespace Windows
{
    struct SoundEngine::SoundBuffer
    {
        SoundBuffer(LPDIRECTSOUNDBUFFER dSoundBuffer)
            : DSoundBuffer { dSoundBuffer }
        {}

        struct LockResult
        {
            bool   succeeded;
            void*  Region1;
            uint32 Region1Size;
            void*  Region2;
            uint32 Region2Size;
        };

        LockResult Lock(uint32 Offset, uint32 Bytes) const
        {
            LockResult result;

            result.succeeded = SUCCEEDED(DSoundBuffer->Lock(Offset,
                                                            Bytes,
                                                            &result.Region1,
                                                            &reinterpret_cast<DWORD&>(result.Region1Size),
                                                            &result.Region2,
                                                            &reinterpret_cast<DWORD&>(result.Region2Size),
                                                            0));
            return result;
        }

        bool Unlock(const LockResult& lock) const
        {
            return lock.succeeded &&
                   SUCCEEDED(DSoundBuffer->Unlock(lock.Region1, lock.Region1Size, lock.Region2, lock.Region2Size));
        }

        void Play() const { DSoundBuffer->Play(0, 0, DSBPLAY_LOOPING); }

        Cursors GetCursors() const
        {
            Cursors cursors;
            DSoundBuffer->GetCurrentPosition(&reinterpret_cast<DWORD&>(cursors.PlayCursor),
                                             &reinterpret_cast<DWORD&>(cursors.WriteCursor));
            return cursors;
        }
        LPDIRECTSOUNDBUFFER DSoundBuffer;
    };

    SoundEngine::SoundEngine() = default;

    SoundEngine ::~SoundEngine() = default;

    void SoundEngine::Init(HWND Window)
    {
        if (auto DSoundLibrary = LoadLibraryA("dsound.dll"))
        {
            using DirectSoundCreateCB = HRESULT (*)(LPCGUID, LPDIRECTSOUND*, LPUNKNOWN);

            auto DirectSoundCreate = (DirectSoundCreateCB)GetProcAddress(DSoundLibrary, "DirectSoundCreate8");

            // Check that this works on XP - DirectSound8 or 7??
            LPDIRECTSOUND DirectSound;
            if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
            {
                WAVEFORMATEX WaveFormat;
                WaveFormat.wFormatTag      = WAVE_FORMAT_PCM;
                WaveFormat.nChannels       = ChannelCount;
                WaveFormat.nSamplesPerSec  = SamplesPerSecond;
                WaveFormat.wBitsPerSample  = BitsPerSample;
                WaveFormat.nBlockAlign     = BlockAlign;
                WaveFormat.nAvgBytesPerSec = SamplesPerSecond * BlockAlign;
                WaveFormat.cbSize          = 0;

                if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
                {
                    DSBUFFERDESC BufferDescription = {};
                    BufferDescription.dwSize       = sizeof(BufferDescription);
                    BufferDescription.dwFlags      = DSBCAPS_PRIMARYBUFFER;

                    // "Create" a primary buffer
                    // DSBCAPS_GLOBALFOCUS?
                    LPDIRECTSOUNDBUFFER PrimaryBuffer;
                    if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, nullptr)))
                    {
                        auto Error = PrimaryBuffer->SetFormat(&WaveFormat);
                        if (SUCCEEDED(Error))
                        {
                            // We have finally set the format!
                            OutputDebugStringA("Primary buffer format was set.\n");
                        }
                        else
                        {
                            // TODO:Diagnostic
                        }
                    }
                    else
                    {
                        // TODO:Diagnostic
                    }
                }
                else
                {
                    // TODO:Diagnostic
                }

                // TODO: DSBCAPS_GETCURRENTPOSITION2
                DSBUFFERDESC BufferDescription  = {};
                BufferDescription.dwSize        = sizeof(BufferDescription);
                BufferDescription.dwFlags       = 0;
                BufferDescription.dwBufferBytes = WorkBufferSize;
                BufferDescription.lpwfxFormat   = &WaveFormat;

                LPDIRECTSOUNDBUFFER dsSoundBuffer;

                auto Error = DirectSound->CreateSoundBuffer(&BufferDescription, &dsSoundBuffer, nullptr);
                if (SUCCEEDED(Error))
                {
                    WorkBuffer = std::make_unique<SoundBuffer>(dsSoundBuffer);
                    OutputDebugStringA("Work buffer created successfully.\n");
                }

                Samples = static_cast<int16*>(
                    VirtualAlloc(nullptr, WorkBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
            }
            else
            {
                // TODO: Diagnostic
            }
        }
        else
        {
            // TODO: Diagnostic
        }
    }

    void SoundEngine::ClearBuffer() const
    {
        if (auto lock = WorkBuffer->Lock(0, WorkBufferSize); lock.succeeded)
        {
            ZeroMemory(lock.Region1, lock.Region1Size);
            ZeroMemory(lock.Region2, lock.Region2Size);

            WorkBuffer->Unlock(lock);
        }
    }

    void SoundEngine::Play() const { WorkBuffer->Play(); }

    Cursors SoundEngine::GetCursors() const { return WorkBuffer->GetCursors(); }

    Game::SoundOutputBuffer SoundEngine::PrepareUpdate()
    {
        ByteToLock   = 0;
        BytesToWrite = 0;

        bool32 SoundIsValid            = false;
        auto [PlayCursor, WriteCursor] = WorkBuffer->GetCursors();
        // if (SUCCEEDED(WorkBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
        {
            ByteToLock         = (RunningSampleIndex * BytesPerSample) % WorkBufferSize;
            DWORD TargetCursor = (PlayCursor + (LatencySampleCount * BytesPerSample)) % WorkBufferSize;

            if (ByteToLock > TargetCursor)
            {
                BytesToWrite = (WorkBufferSize - ByteToLock);
                BytesToWrite += TargetCursor;
            }
            else
            {
                BytesToWrite = TargetCursor - ByteToLock;
            }

            SoundIsValid = true;
#if DEBUG_SOUND
            lastPlayCursor   = PlayCursor;
            lastWriteCursor  = WriteCursor;
            lastByteToLock   = ByteToLock;
            lastBytesToWrite = BytesToWrite;
#endif // DEBUG_SOUND
        }

        return { SamplesPerSecond, BytesToWrite / BytesPerSample, Samples, SoundIsValid };
    }

    void SoundEngine::FillSoundBuffer(Game::SoundOutputBuffer& SourceBuffer)
    {
        if (auto lock = WorkBuffer->Lock(ByteToLock, BytesToWrite); lock.succeeded)
        {
            CopyMemory(lock.Region1, SourceBuffer.Samples, lock.Region1Size);
            CopyMemory(lock.Region2, (uint8*)SourceBuffer.Samples + lock.Region1Size, lock.Region2Size);
            RunningSampleIndex += (lock.Region1Size + lock.Region2Size) / BytesPerSample;

            WorkBuffer->Unlock(lock);
        }
    }
} // namespace Windows