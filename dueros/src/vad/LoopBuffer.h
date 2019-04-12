#ifndef __LOOP_BUFFER_H__
#define __LOOP_BUFFER_H__

#include "types.h"
#include "helper.h"
#include <vector>
#include <string.h>

// Single thread loop buffer.
// NOT thread safe!
template <typename T>
class LoopBuffer
{
    public:
        LoopBuffer(int buffer_size) :
                _buffer_size(buffer_size),
                _avail_data_size(0),
                _write_pos(0),
                _read_pos(0),
                _debug_mode(0) {
            _buffer.resize(_buffer_size);
            _buffer_ptr = &(_buffer[0]);
        }

        int Read(T* data_out, int size, int consume_size = -1) {
            if (_avail_data_size - size < 0) {
                MDNN_LOG_VERBOSE("buffer empty");
                return 0;
            }

            if (consume_size < 0) {
                consume_size = size;
            }

            if (_debug_mode == 1) {
                MDNN_LOG_VERBOSE("Read(), _buffer_ptr: %p, _read_pos: %d, data_out: %p, size: %d", _buffer_ptr, _read_pos, data_out, size);
            }
            if (_read_pos + size > _buffer_size) {
                int first_part_size = _buffer_size - _read_pos;
                memcpy(data_out, _buffer_ptr + _read_pos, first_part_size * sizeof(T));
                int second_part_size = size - first_part_size;
                memcpy(data_out + first_part_size, _buffer_ptr, second_part_size * sizeof(T));

                if (_read_pos + consume_size > _buffer_size) {
                    _read_pos = consume_size - (_buffer_size - _read_pos);
                } else {
                    _read_pos += consume_size;
                }
            } else {
                memcpy(data_out, _buffer_ptr + _read_pos, size * sizeof(T));
                _read_pos += consume_size;
            }

            _avail_data_size -= consume_size;
            if (_debug_mode == 1) {
                MDNN_LOG_VERBOSE("_avail_data_size: %d", _avail_data_size);
            }
            return size;
        }

        int Write(T* data_in, int size) {
            if (_avail_data_size + size > _buffer_size) {
                MDNN_LOG_VERBOSE("buffer full");
                return 0;
            }

            if (_debug_mode == 1) {
                MDNN_LOG_VERBOSE("Write(), _buffer_ptr: %p, _write_pos: %d, data_in: %p, size: %d", _buffer_ptr, _write_pos, data_in, size);
            }
            if (_write_pos + size > _buffer_size) {
                int first_part_size = _buffer_size - _write_pos;
                memcpy(_buffer_ptr + _write_pos, data_in, first_part_size * sizeof(T));
                int second_part_size = size - first_part_size;
                memcpy(_buffer_ptr, data_in + first_part_size, second_part_size * sizeof(T));
                _write_pos = second_part_size;
            } else {
                memcpy(_buffer_ptr + _write_pos, data_in, size * sizeof(T));
                _write_pos += size;
            }

            _avail_data_size += size;
            if (_debug_mode == 1) {
                MDNN_LOG_VERBOSE("_avail_data_size: %d", _avail_data_size);
            }
            return size;
        }

        int AvailableData() {
            return _avail_data_size;
        }

        void SetDebugMode(int debug_mode) {
            _debug_mode = debug_mode;
        }

    private:
        int _buffer_size;
        int _avail_data_size;
        int _write_pos;
        int _read_pos;
        std::vector<T> _buffer;
        T* _buffer_ptr;
        int _debug_mode;

        inline int compute_wrap(int pos, int incr) {
            return (pos + incr) % _buffer_size;
        }
};
#endif
