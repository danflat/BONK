#include "bonk/ble_tx_mux.hpp"

#include <algorithm>
#include <cstring>

namespace bonk {

bool BleTxMux::enqueue(
    BleChannel channel,
    const std::uint8_t *data,
    std::size_t size) noexcept {
    if (data == nullptr || size == 0 || size > BleFrame::kMaxPayload) {
        return false;
    }

    std::uint32_t sequence = next_sequence_++;
    if (sequence == 0) {
        sequence = next_sequence_++;
    }

    if (channel == BleChannel::FlarmTelemetry) {
        if (flarm_.count == kQueueDepth) {
            flarm_.head = (flarm_.head + 1) % kQueueDepth;
            --flarm_.count;
            ++stats_.dropped_flarm;
        }
        const bool accepted =
            push(flarm_, channel, data, size, sequence);
        if (accepted) {
            ++stats_.queued_flarm;
        }
        return accepted;
    }

    if (mesh_.count == kQueueDepth) {
        ++stats_.dropped_mesh;
        return false;
    }
    const bool accepted = push(mesh_, channel, data, size, sequence);
    if (accepted) {
        ++stats_.queued_mesh;
    }
    return accepted;
}

bool BleTxMux::enqueueText(
    BleChannel channel,
    const char *text) noexcept {
    if (text == nullptr) {
        return false;
    }
    return enqueue(
        channel,
        reinterpret_cast<const std::uint8_t *>(text),
        std::strlen(text));
}

bool BleTxMux::pop(BleFrame &frame) noexcept {
    if (pull(flarm_, frame)) {
        return true;
    }
    return pull(mesh_, frame);
}

std::size_t BleTxMux::pending(BleChannel channel) const noexcept {
    return channel == BleChannel::FlarmTelemetry
               ? flarm_.count
               : mesh_.count;
}

const BleMuxStats &BleTxMux::stats() const noexcept {
    return stats_;
}

bool BleTxMux::push(
    Queue &queue,
    BleChannel channel,
    const std::uint8_t *data,
    std::size_t size,
    std::uint32_t sequence) noexcept {
    if (queue.count >= kQueueDepth) {
        return false;
    }

    BleFrame &frame = queue.frames[queue.tail];
    frame = {};
    frame.channel = channel;
    frame.size = size;
    frame.sequence = sequence;
    std::copy_n(data, size, frame.payload.begin());

    queue.tail = (queue.tail + 1) % kQueueDepth;
    ++queue.count;
    return true;
}

bool BleTxMux::pull(
    Queue &queue,
    BleFrame &frame) noexcept {
    if (queue.count == 0) {
        return false;
    }

    frame = queue.frames[queue.head];
    queue.head = (queue.head + 1) % kQueueDepth;
    --queue.count;
    return true;
}

} // namespace bonk
