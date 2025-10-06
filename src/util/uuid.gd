#MIT License

#Copyright (c) 2023 Xavier Sellier

#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:

#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#SOFTWARE.

# uuid.gd
@tool
class_name UUID
extends Resource

const BYTE_MASK: int = 0b11111111

@export var uuid : Array[int]
@export var uuid_string : String = ""
#-------------------------------------------------------------------------------
static func uuidbin() -> Array[int]:
	randomize()
	# 16 random bytes with the bytes on index 6 and 8 modified
	return [
	randi() & BYTE_MASK, randi() & BYTE_MASK, randi() & BYTE_MASK, randi() & BYTE_MASK,
	randi() & BYTE_MASK, randi() & BYTE_MASK, ((randi() & BYTE_MASK) & 0x0f) | 0x40, randi() & BYTE_MASK,
	((randi() & BYTE_MASK) & 0x3f) | 0x80, randi() & BYTE_MASK, randi() & BYTE_MASK, randi() & BYTE_MASK,
	randi() & BYTE_MASK, randi() & BYTE_MASK, randi() & BYTE_MASK, randi() & BYTE_MASK,
	]

static func uuidbinrng(rng: RandomNumberGenerator) -> Array[int]:
	rng.randomize()
	return [
	rng.randi() & BYTE_MASK, rng.randi() & BYTE_MASK, rng.randi() & BYTE_MASK, rng.randi() & BYTE_MASK,
	rng.randi() & BYTE_MASK, rng.randi() & BYTE_MASK, ((rng.randi() & BYTE_MASK) & 0x0f) | 0x40, rng.randi() & BYTE_MASK,
	((rng.randi() & BYTE_MASK) & 0x3f) | 0x80, rng.randi() & BYTE_MASK, rng.randi() & BYTE_MASK, rng.randi() & BYTE_MASK,
	rng.randi() & BYTE_MASK, rng.randi() & BYTE_MASK, rng.randi() & BYTE_MASK, rng.randi() & BYTE_MASK,
	]
#-------------------------------------------------------------------------------
func _init() -> void:
	uuid = UUID.uuidbin()
	uuid_string = get_string()
#-------------------------------------------------------------------------------
func get_string() -> String:
	if not uuid_string.is_empty():
		return uuid_string
	return '%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x' % [
	# low
	uuid[0], uuid[1], uuid[2], uuid[3],

	# mid
	uuid[4], uuid[5],

	# hi
	uuid[6], uuid[7],

	# clock
	uuid[8], uuid[9],

	# node
	uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]
	]

func get_snake_case_string() -> String:
	if not uuid_string.is_empty():
		return uuid_string.to_snake_case()
	return '%02x%02x%02x%02x_%02x%02x_%02x%02x_%02x%02x_%02x%02x%02x%02x%02x%02x' % [
	# low
	uuid[0], uuid[1], uuid[2], uuid[3],

	# mid
	uuid[4], uuid[5],

	# hi
	uuid[6], uuid[7],

	# clock
	uuid[8], uuid[9],

	# node
	uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]
	]

func get_array() -> Array[int]:
	return uuid.duplicate()

func get_dict(big_endian := true) -> Dictionary[String, int]:
	if big_endian:
		return {
		"low"    	: (uuid[0]    	<< 24) + 	(uuid[1]    << 16) + (uuid[2]    << 8 ) +    uuid[3],
		"mid"    	: (uuid[4]    	<< 8 ) +    uuid[5],
		"hi"     	: (uuid[6]    	<< 8 ) +    uuid[7],
		"clock"		: (uuid[8]    	<< 8 ) +    uuid[9],
		"node" 		: (uuid[10] 	<< 40) + 	(uuid[11] << 32) + (uuid[12] << 24) + (uuid[13] << 16) + (uuid[14] << 8 ) +    uuid[15]
	}
	else:
		return {
		"low"    	: uuid[0]                    	+ (uuid[1]    << 8 ) + (uuid[2]    << 16) + (uuid[3]    << 24),
		"mid"    	: uuid[4]                    	+ (uuid[5]    << 8 ),
		"hi"     	: uuid[6]                    	+ (uuid[7]    << 8 ),
		"clock"		: uuid[8]                    	+ (uuid[9]    << 8 ),
		"node" 		: uuid[10]                 	+ (uuid[11] << 8 ) + (uuid[12] << 16) + (uuid[13] << 24) + (uuid[14] << 32) + (uuid[15] << 40)
	}
#-------------------------------------------------------------------------------
func is_equal(other: UUID) -> bool:
	# Godot Engine compares Array recursively
	# There's no need for custom comparison here.
	return uuid == other.uuid
