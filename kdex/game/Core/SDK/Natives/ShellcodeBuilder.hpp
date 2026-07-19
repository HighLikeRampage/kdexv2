#pragma once
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <utility>
#include <vector>

class ShellcodeBuilder {
public:
    struct Label {
        long long bound = -1;
        std::vector<size_t> pending;
    };

    class Asm {
    public:
        std::vector<uint8_t> code;

        size_t at() const { return code.size(); }

        void e(std::initializer_list<uint8_t> bs) {
            for (auto b : bs) code.push_back(b);
        }
        void e1(uint8_t v) { code.push_back(v); }
        void e32(int32_t v) {
            code.push_back(uint8_t(v));
            code.push_back(uint8_t(v >> 8));
            code.push_back(uint8_t(v >> 16));
            code.push_back(uint8_t(v >> 24));
        }
        void e64(uint64_t v) {
            for (int i = 0; i < 8; ++i) code.push_back(uint8_t(v >> (i * 8)));
        }

        void riprel(std::initializer_list<uint8_t> op, size_t dOff) {
            for (auto b : op) code.push_back(b);
            m_rip.push_back({ code.size(), dOff });
            e32(0);
        }

        size_t movabsImm(std::initializer_list<uint8_t> prefix) {
            for (auto b : prefix) code.push_back(b);
            const size_t off = code.size();
            e64(0);
            return off;
        }
        void patchImm64(size_t off, uint64_t v) {
            for (int i = 0; i < 8; ++i) code[off + i] = uint8_t(v >> (i * 8));
        }

        void nearJmp(Label& L) {
            code.push_back(0xE9);
            m_near.push_back({ code.size(), L.bound });
            e32(0);
            if (L.bound < 0) L.pending.push_back(m_near.size() - 1);
        }
        void nearJz(Label& L) {
            code.push_back(0x0F); code.push_back(0x84);
            m_near.push_back({ code.size(), L.bound });
            e32(0);
            if (L.bound < 0) L.pending.push_back(m_near.size() - 1);
        }
        void nearJae(Label& L) {
            code.push_back(0x0F); code.push_back(0x83);
            m_near.push_back({ code.size(), L.bound });
            e32(0);
            if (L.bound < 0) L.pending.push_back(m_near.size() - 1);
        }

        void bind(Label& L) {
            L.bound = static_cast<long long>(code.size());
            for (auto idx : L.pending) m_near[idx].target = L.bound;
            L.pending.clear();
        }

        struct Built {
            std::vector<uint8_t> buffer;
            size_t dataOff;
        };
        Built finalize(size_t dataSize) {
            const size_t dataOff = (code.size() + 15) & ~size_t(15);
            Built out{ std::vector<uint8_t>(dataOff + dataSize, 0), dataOff };
            std::memcpy(out.buffer.data(), code.data(), code.size());
            auto writeI32 = [&](size_t at, int32_t v) {
                out.buffer[at + 0] = uint8_t(v);
                out.buffer[at + 1] = uint8_t(v >> 8);
                out.buffer[at + 2] = uint8_t(v >> 16);
                out.buffer[at + 3] = uint8_t(v >> 24);
            };
            for (const auto& r : m_rip) {
                long long nextInst = static_cast<long long>(r.opEnd + 4);
                long long disp = static_cast<long long>(dataOff + r.dOff) - nextInst;
                writeI32(r.opEnd, static_cast<int32_t>(disp));
            }
            for (const auto& n : m_near) {
                long long nextInst = static_cast<long long>(n.immAt + 4);
                long long disp = n.target - nextInst;
                writeI32(n.immAt, static_cast<int32_t>(disp));
            }
            return out;
        }

    private:
        struct RipFix { size_t opEnd; size_t dOff; };
        struct NearFix { size_t immAt; long long target; };
        std::vector<RipFix> m_rip;
        std::vector<NearFix> m_near;
    };

    struct CitizenShellcode {
        std::vector<uint8_t> buffer;
        size_t PATCH_QUEUE;
        size_t PATCH_ORIGFUNC;
    };

    struct DirectCallShellcode {
        std::vector<uint8_t> buffer;
        size_t dataOff;
        size_t D_STACK_TOP, D_SAVED_RAX, D_SAVED_RSP, D_SAVED_RIP;
        size_t D_HANDLER, D_ARGCOUNT, D_DONE, D_RESULT, D_ARG, D_SIZE;
    };

    struct MainFnShellcode {
        std::vector<uint8_t> buffer;
        size_t dataOff;
        size_t D_FLAG, D_DONE, D_HASH, D_ARGCOUNT;
        size_t D_INIT, D_PUSH, D_CALL, D_WAIT;
        size_t D_RESULT, D_ARG, D_SIZE;
    };

    struct BootstrapShellcode {
        std::vector<uint8_t> buffer;
        size_t dataOff;
        size_t D_STACK_TOP, D_SAVED_RAX, D_SAVED_RSP, D_SAVED_RIP, D_SIZE;
    };

    struct ApcCallShellcode {
        std::vector<uint8_t> buffer;
        size_t D_HANDLER, D_ARGCOUNT, D_DONE, D_RESULT, D_PENDING, D_ARG, D_SIZE;
    };

    static CitizenShellcode  buildCitizenShellcode();
    static DirectCallShellcode buildDirectCall();
    static MainFnShellcode   buildMainFn();
    static BootstrapShellcode buildBootstrap(uint64_t remoteFnVA, uint64_t arg1, uint64_t arg2);
    static ApcCallShellcode  buildApcCall();
};

inline ShellcodeBuilder::CitizenShellcode ShellcodeBuilder::buildCitizenShellcode() {
    CitizenShellcode r;
    r.buffer = {
        0x9C,
        0x50, 0x51,
        0x48, 0xBA, 0,0,0,0,0,0,0,0,
        0x52,
        0x48, 0x83, 0x82, 0xC0,0x00,0x00,0x00, 0x01,
        0x0F, 0xB6, 0x02,
        0x85, 0xC0,
        0x0F, 0x84, 0x40, 0x00, 0x00, 0x00,
        0xC6, 0x02, 0x00,
        0x4C, 0x8D, 0x92, 0xA0,0x00,0x00,0x00,
        0x4C, 0x8D, 0x9A, 0xC8,0x00,0x00,0x00,
        0x4D, 0x89, 0x1A,
        0x8B, 0x4A, 0x18,
        0x41, 0x89, 0x4A, 0x08,
        0x48, 0x8D, 0x42, 0x20,
        0x49, 0x89, 0x42, 0x10,
        0x48, 0x8B, 0x42, 0x08,
        0x48, 0x8D, 0x8A, 0xA0,0x00,0x00,0x00,
        0x48, 0x83, 0xEC, 0x28,
        0xFF, 0xD0,
        0x48, 0x83, 0xC4, 0x28,
        0x48, 0x8B, 0x14, 0x24,
        0xC6, 0x42, 0x01, 0x01,
        0x5A, 0x59, 0x58, 0x9D,
        0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,
        0,0,0,0,0,0,0,0,
    };
    r.PATCH_QUEUE = 5;
    r.PATCH_ORIGFUNC = 107;
    return r;
}

inline ShellcodeBuilder::DirectCallShellcode ShellcodeBuilder::buildDirectCall() {
    constexpr size_t D_STACK_TOP = 0x00, D_SAVED_RAX = 0x08, D_SAVED_RSP = 0x10, D_SAVED_RIP = 0x18;
    constexpr size_t D_HANDLER   = 0x20, D_ARGCOUNT  = 0x28, D_DONE       = 0x30;
    constexpr size_t D_RESULT    = 0x40, D_ARG       = 0x60, D_SIZE       = 0xA0;

    Asm a;

    a.riprel({0x48, 0x89, 0x05}, D_SAVED_RAX);
    a.riprel({0x48, 0x89, 0x25}, D_SAVED_RSP);
    a.riprel({0x48, 0x8B, 0x05}, D_STACK_TOP);
    a.e({0x48, 0x89, 0xC4});

    a.e({0x55, 0x9C});
    a.e({0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53});
    a.e({0x53, 0x56, 0x57});
    a.e({0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57});

    a.e({0x48, 0x83, 0xEC, 0x60});
    a.e({0x0F, 0x29, 0x04, 0x24});
    a.e({0x0F, 0x29, 0x4C, 0x24, 0x10});
    a.e({0x0F, 0x29, 0x54, 0x24, 0x20});
    a.e({0x0F, 0x29, 0x5C, 0x24, 0x30});
    a.e({0x0F, 0x29, 0x64, 0x24, 0x40});
    a.e({0x0F, 0x29, 0x6C, 0x24, 0x50});

    a.e({0x48, 0x81, 0xEC, 0xB0, 0x00, 0x00, 0x00});

    a.e({0x31, 0xC0});
    a.e({0x48, 0x89, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00});
    a.e({0x48, 0x89, 0x84, 0x24, 0x88, 0x00, 0x00, 0x00});
    a.e({0x48, 0x89, 0x84, 0x24, 0x90, 0x00, 0x00, 0x00});
    a.e({0x48, 0x89, 0x84, 0x24, 0x98, 0x00, 0x00, 0x00});

    a.e({0x48, 0x8D, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00});
    a.e({0x48, 0x89, 0x44, 0x24, 0x20});

    a.riprel({0x48, 0x8B, 0x05}, D_ARGCOUNT);
    a.e({0x89, 0x44, 0x24, 0x28});

    a.e({0x48, 0x8D, 0x44, 0x24, 0x40});
    a.e({0x48, 0x89, 0x44, 0x24, 0x30});
    a.e({0xC7, 0x44, 0x24, 0x38, 0x00, 0x00, 0x00, 0x00});

    static constexpr uint8_t argOffs[8] = { 0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78 };
    for (int i = 0; i < 8; i++) {
        a.riprel({0x48, 0x8B, 0x05}, D_ARG + i * 8);
        a.e({0x48, 0x89, 0x44, 0x24, argOffs[i]});
    }

    a.e({0x48, 0x8D, 0x4C, 0x24, 0x20});
    a.riprel({0x48, 0x8B, 0x05}, D_HANDLER);
    a.e({0xFF, 0xD0});

    a.e({0x48, 0x8B, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00});
    a.riprel({0x48, 0x89, 0x05}, D_RESULT + 0);
    a.e({0x48, 0x8B, 0x84, 0x24, 0x88, 0x00, 0x00, 0x00});
    a.riprel({0x48, 0x89, 0x05}, D_RESULT + 8);
    a.e({0x48, 0x8B, 0x84, 0x24, 0x90, 0x00, 0x00, 0x00});
    a.riprel({0x48, 0x89, 0x05}, D_RESULT + 16);

    a.e({0x48, 0x81, 0xC4, 0xB0, 0x00, 0x00, 0x00});

    a.e({0x0F, 0x28, 0x04, 0x24});
    a.e({0x0F, 0x28, 0x4C, 0x24, 0x10});
    a.e({0x0F, 0x28, 0x54, 0x24, 0x20});
    a.e({0x0F, 0x28, 0x5C, 0x24, 0x30});
    a.e({0x0F, 0x28, 0x64, 0x24, 0x40});
    a.e({0x0F, 0x28, 0x6C, 0x24, 0x50});
    a.e({0x48, 0x83, 0xC4, 0x60});

    a.e({0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C});
    a.e({0x5F, 0x5E, 0x5B});
    a.e({0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59});
    a.e({0x9D});
    a.e({0x5D});

    a.riprel({0xF0, 0x48, 0xFF, 0x05}, D_DONE);
    a.riprel({0x48, 0x8B, 0x25},       D_SAVED_RSP);
    a.riprel({0x48, 0x8B, 0x05},       D_SAVED_RAX);
    a.riprel({0xFF, 0x25},             D_SAVED_RIP);

    auto built = a.finalize(D_SIZE);
    DirectCallShellcode r{};
    r.buffer      = std::move(built.buffer);
    r.dataOff     = built.dataOff;
    r.D_STACK_TOP = D_STACK_TOP; r.D_SAVED_RAX = D_SAVED_RAX;
    r.D_SAVED_RSP = D_SAVED_RSP; r.D_SAVED_RIP = D_SAVED_RIP;
    r.D_HANDLER   = D_HANDLER;   r.D_ARGCOUNT  = D_ARGCOUNT;   r.D_DONE = D_DONE;
    r.D_RESULT    = D_RESULT;    r.D_ARG       = D_ARG;        r.D_SIZE = D_SIZE;
    return r;
}

inline ShellcodeBuilder::MainFnShellcode ShellcodeBuilder::buildMainFn() {
    constexpr size_t D_FLAG = 0x00, D_DONE = 0x08, D_HASH = 0x10, D_ARGCOUNT = 0x18;
    constexpr size_t D_INIT = 0x20, D_PUSH = 0x28, D_CALL = 0x30, D_WAIT     = 0x38;
    constexpr size_t D_RESULT = 0x40, D_ARG = 0x60, D_SIZE = 0xA0;

    Asm a;
    Label loop, yield;

    a.e({0x55});
    a.e({0x48, 0x89, 0xE5});
    a.e({0x48, 0x83, 0xEC, 0x30});

    a.bind(loop);

    a.riprel({0x8A, 0x05}, D_FLAG);
    a.e({0x84, 0xC0});
    a.nearJz(yield);

    a.e({0x31, 0xC0});
    a.riprel({0x86, 0x05}, D_FLAG);
    a.e({0x84, 0xC0});
    a.nearJz(yield);

    a.riprel({0x48, 0x8B, 0x0D}, D_HASH);
    a.riprel({0x48, 0x8B, 0x05}, D_INIT);
    a.e({0xFF, 0xD0});

    a.e({0x4D, 0x31, 0xD2});

    Label pushLoop, pushDone;
    a.bind(pushLoop);
    a.riprel({0x4C, 0x8B, 0x1D}, D_ARGCOUNT);
    a.e({0x4D, 0x39, 0xDA});
    a.nearJae(pushDone);
    a.riprel({0x48, 0x8D, 0x05}, D_ARG);
    a.e({0x4A, 0x8B, 0x0C, 0xD0});
    a.riprel({0x48, 0x8B, 0x05}, D_PUSH);
    a.e({0xFF, 0xD0});
    a.e({0x49, 0xFF, 0xC2});
    a.nearJmp(pushLoop);

    a.bind(pushDone);
    a.riprel({0x48, 0x8B, 0x05}, D_CALL);
    a.e({0xFF, 0xD0});

    a.e({0x48, 0x8B, 0x08});
    a.riprel({0x48, 0x89, 0x0D}, D_RESULT + 0);
    a.e({0x48, 0x8B, 0x48, 0x08});
    a.riprel({0x48, 0x89, 0x0D}, D_RESULT + 8);
    a.e({0x48, 0x8B, 0x48, 0x10});
    a.riprel({0x48, 0x89, 0x0D}, D_RESULT + 16);

    a.riprel({0xF0, 0x48, 0xFF, 0x05}, D_DONE);

    a.bind(yield);
    a.e({0x31, 0xC9});
    a.riprel({0x48, 0x8B, 0x05}, D_WAIT);
    a.e({0xFF, 0xD0});
    a.nearJmp(loop);

    auto built = a.finalize(D_SIZE);
    MainFnShellcode r{};
    r.buffer    = std::move(built.buffer);
    r.dataOff   = built.dataOff;
    r.D_FLAG    = D_FLAG;    r.D_DONE     = D_DONE;
    r.D_HASH    = D_HASH;    r.D_ARGCOUNT = D_ARGCOUNT;
    r.D_INIT    = D_INIT;    r.D_PUSH     = D_PUSH;
    r.D_CALL    = D_CALL;    r.D_WAIT     = D_WAIT;
    r.D_RESULT  = D_RESULT;  r.D_ARG      = D_ARG;
    r.D_SIZE    = D_SIZE;
    return r;
}

inline ShellcodeBuilder::BootstrapShellcode ShellcodeBuilder::buildBootstrap(uint64_t remoteFnVA, uint64_t arg1, uint64_t arg2) {
    constexpr size_t D_STACK_TOP = 0x00, D_SAVED_RAX = 0x08, D_SAVED_RSP = 0x10, D_SAVED_RIP = 0x18;
    constexpr size_t D_SIZE      = 0x20;

    Asm a;

    a.riprel({0x48, 0x89, 0x05}, D_SAVED_RAX);
    a.riprel({0x48, 0x89, 0x25}, D_SAVED_RSP);
    a.riprel({0x48, 0x8B, 0x05}, D_STACK_TOP);
    a.e({0x48, 0x89, 0xC4});

    a.e({0x55});
    a.e({0x9C});
    a.e({0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53});
    a.e({0x53, 0x56, 0x57});
    a.e({0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57});
    a.e({0x48, 0x83, 0xEC, 0x28});

    const size_t arg1Off = a.movabsImm({0x48, 0xB9});
    const size_t arg2Off = a.movabsImm({0x48, 0xBA});
    const size_t fnOff   = a.movabsImm({0x48, 0xB8});
    a.e({0xFF, 0xD0});
    a.e({0x48, 0x83, 0xC4, 0x28});

    a.e({0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C});
    a.e({0x5F, 0x5E, 0x5B});
    a.e({0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59});
    a.e({0x9D});
    a.e({0x5D});

    a.riprel({0x48, 0x8B, 0x25}, D_SAVED_RSP);
    a.riprel({0x48, 0x8B, 0x05}, D_SAVED_RAX);
    a.riprel({0xFF, 0x25},       D_SAVED_RIP);

    a.patchImm64(arg1Off, arg1);
    a.patchImm64(arg2Off, arg2);
    a.patchImm64(fnOff,   remoteFnVA);

    auto built = a.finalize(D_SIZE);
    BootstrapShellcode r{};
    r.buffer      = std::move(built.buffer);
    r.dataOff     = built.dataOff;
    r.D_STACK_TOP = D_STACK_TOP; r.D_SAVED_RAX = D_SAVED_RAX;
    r.D_SAVED_RSP = D_SAVED_RSP; r.D_SAVED_RIP = D_SAVED_RIP;
    r.D_SIZE      = D_SIZE;
    return r;
}

inline ShellcodeBuilder::ApcCallShellcode ShellcodeBuilder::buildApcCall() {
    constexpr uint8_t D_HANDLER  = 0x00, D_ARGCOUNT = 0x08, D_DONE     = 0x10;
    constexpr uint8_t D_RESULT   = 0x18, D_PENDING  = 0x30, D_ARG      = 0x38;
    constexpr size_t  D_SIZE     = 0x78;

    std::vector<uint8_t> body;
    auto bemit = [&](std::initializer_list<uint8_t> bs) { for (auto b : bs) body.push_back(b); };

    for (uint8_t i = 0; i < 7; i++) {
        bemit({0x49, 0x8B, 0x47, uint8_t(D_ARG + i * 8)});
        bemit({0x48, 0x89, 0x44, 0x24, uint8_t(0x48 + i * 8)});
    }
    bemit({0x49, 0x8B, 0x47, uint8_t(D_ARG + 7 * 8)});
    bemit({0x48, 0x89, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00});

    bemit({0x31, 0xC0});
    bemit({0x48, 0x89, 0x44, 0x24, 0x20});
    bemit({0x48, 0x8D, 0x44, 0x24, 0x48});
    bemit({0x48, 0x89, 0x44, 0x24, 0x28});
    bemit({0x48, 0x89, 0x44, 0x24, 0x38});
    bemit({0x49, 0x8B, 0x47, D_ARGCOUNT});
    bemit({0x89, 0x44, 0x24, 0x30});
    bemit({0xC7, 0x44, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00});

    bemit({0x48, 0x8D, 0x4C, 0x24, 0x20});
    bemit({0x49, 0x8B, 0x07});
    bemit({0xFF, 0xD0});

    bemit({0x48, 0x8B, 0x44, 0x24, 0x48});
    bemit({0x49, 0x89, 0x47, D_RESULT});
    bemit({0x48, 0x8B, 0x44, 0x24, 0x50});
    bemit({0x49, 0x89, 0x47, uint8_t(D_RESULT + 8)});
    bemit({0x48, 0x8B, 0x44, 0x24, 0x58});
    bemit({0x49, 0x89, 0x47, uint8_t(D_RESULT + 16)});

    bemit({0xF0, 0x49, 0xFF, 0x47, D_DONE});

    ApcCallShellcode r{};
    auto& out = r.buffer;
    auto emit = [&](std::initializer_list<uint8_t> bs) { for (auto b : bs) out.push_back(b); };

    emit({0x55});
    emit({0x48, 0x89, 0xE5});
    emit({0x41, 0x57});
    emit({0x41, 0x56});
    emit({0x48, 0x81, 0xEC, 0xB8, 0x00, 0x00, 0x00});

    emit({0x49, 0x89, 0xCF});

    emit({0x31, 0xC0});
    emit({0xF0, 0x41, 0x87, 0x47, D_PENDING});
    emit({0x85, 0xC0});
    const int32_t jzDisp = static_cast<int32_t>(body.size());
    emit({0x0F, 0x84,
          uint8_t(jzDisp), uint8_t(jzDisp >> 8),
          uint8_t(jzDisp >> 16), uint8_t(jzDisp >> 24)});

    out.insert(out.end(), body.begin(), body.end());

    emit({0x48, 0x81, 0xC4, 0xB8, 0x00, 0x00, 0x00});
    emit({0x41, 0x5E});
    emit({0x41, 0x5F});
    emit({0x5D});
    emit({0xC3});

    r.D_HANDLER  = D_HANDLER; r.D_ARGCOUNT = D_ARGCOUNT; r.D_DONE   = D_DONE;
    r.D_RESULT   = D_RESULT;  r.D_PENDING  = D_PENDING;  r.D_ARG    = D_ARG;
    r.D_SIZE     = D_SIZE;
    return r;
}
