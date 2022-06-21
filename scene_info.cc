#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include <map>
#include <sstream>

#include "z64.h"

bool FlagDumpSceneLocations = false;
uint64_t FlagSceneToDump = -1;

enum ROM {
	kDebug = 0,
};

enum SceneHeaderCommands {

	kCollision = 0x03,
	kRooms = 0x04,

	kRoomMesh = 0x0A,
};

using byte_t = uint8_t;

template<typename T>
T be2le(T be) {
	T ret = 0;
	auto s = sizeof(T);
	for (int i = 0; i < s; ++i) {
		auto shift = ((s - (i + 1)) * 8);
		T mask = 0xFF << shift;

		byte_t b = ((be & mask) >> shift);
		ret |= (b << (i * 8));
	}
	return ret;
}

template<typename T, typename TMem>
T read(const TMem& mem, uint64_t loc) {
	return *((T*)(&mem[loc]));
}

template<typename T>
T str_to(const std::string& str) {
	std::stringstream ss;
	ss << str;
	T ret;
	ss >> ret;
	return ret;
}

struct scene_table_vrom_location_t {
	uint64_t start_, end_;
};

const scene_table_vrom_location_t kSceneTableLocations[] = {
	{ 0x00BA0BB0, 0x00BA1448 },
};

template<typename TMem>
z64_scene_command_t read_scene_command(const TMem& mem, uint64_t loc) {
	return {
		mem[loc],
		mem[loc + 1],
		{ // Padding
			mem[loc + 2],
			mem[loc + 3]
		},
		be2le(read<int32_t>(mem, loc + 4)),
	};
}

uint64_t seg2vrom(int32_t seg, uint32_t scene_vrom_start) {
	assert((seg & 0xFF000000) == 0x02000000);
	return scene_vrom_start + (seg & 0x00FFFFFF);
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Provide a filename" << std::endl;
		return -1;
	}

	for (int i = 1; i < (argc -1); ++i) {
		if (std::string(argv[i]) == "--scene-locations") FlagDumpSceneLocations = true;
		if (std::string(argv[i]) == "--scene") {
			FlagSceneToDump = str_to<uint64_t>(argv[++i]);
		}
	}

    std::ifstream file(std::string(argv[argc - 1]), std::ios::binary);
    std::vector<byte_t> le_rom((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (le_rom[0x2d00000] != 0x15 || le_rom[0x2d00001] != 0x00 || le_rom[0x2d00008] != 0x04 || le_rom[0x2d00009] != 0x01) {

		std::cerr << "Doesn't match: " << std::hex 
				  << static_cast<int>(le_rom[0x2d00000]) << " " 
				  << static_cast<int>(le_rom[0x2d00001]) << " " 
				  << static_cast<int>(le_rom[0x2d00008]) << " " 
				  << static_cast<int>(le_rom[0x2d00009]) << std::endl;
		return -1;
    }

    int32_t test = be2le(read<int32_t>(le_rom, 0xBA0BB0));
    if (test != 0x01FC2000) {
    	std::cerr << "Doesn't match: " << std::hex
    			  << test << std::endl;
    }

    assert((kSceneTableLocations[kDebug].end_ - kSceneTableLocations[kDebug].start_) % sizeof(z64_scene_table_t) == 0);
    std::vector<z64_scene_table_t> scene_table_entries;
    for (uint64_t i = kSceneTableLocations[kDebug].start_; i < kSceneTableLocations[kDebug].end_; i += sizeof(z64_scene_table_t)) {
    	z64_scene_table_t entry = {
    		be2le(read<uint32_t>(le_rom, i)),
    		be2le(read<uint32_t>(le_rom, i + 4)),
    		be2le(read<uint32_t>(le_rom, i + 8)),
    		be2le(read<uint32_t>(le_rom, i + 12)),
    		read<char>(le_rom, i + 17),
    		read<uint8_t>(le_rom, i + 18),
    		read<char>(le_rom, i + 19),
    		read<char>(le_rom, i + 20),
    	};

    	uint64_t idx = (i - kSceneTableLocations[kDebug].start_) / sizeof(z64_scene_table_t);
    	if (FlagDumpSceneLocations) {
	    	std::cout << "Scene: " << std::dec << idx << std::endl << std::hex
	    			  << "\t" << "VROM Start: 0x" << entry.scene_vrom_start << std::endl
	    			  << "\t" << "VROM End:   0x" << entry.scene_vrom_end << std::endl;
    	}

    	scene_table_entries.push_back(entry);
    }

    std::cout << "Found " << std::dec << scene_table_entries.size() << " scene table_entries" << std::endl;

    if (FlagSceneToDump >= 0) {
    	std::cout << "Dumping scene " << FlagSceneToDump << std::endl;
    	uint64_t scene_command_offset = scene_table_entries[FlagSceneToDump].scene_vrom_start;
    	z64_scene_command_t command;
    	do {
    		command = read_scene_command(le_rom, scene_command_offset);

    		switch (command.code) {
    			case kCollision: {
    				uint64_t collision_header_addr = seg2vrom(command.data2, scene_table_entries[FlagSceneToDump].scene_vrom_start);
    				int num_polys = be2le(read<int16_t>(le_rom, collision_header_addr + 0x14));
    				uint64_t poly_array_start =seg2vrom(
    					be2le(read<int32_t>(le_rom, collision_header_addr + 0x18)),
    					scene_table_entries[FlagSceneToDump].scene_vrom_start);
    				std::cout << "Collision:    " << std::endl;
    				std::cout << "N vertices:   " << be2le(read<int16_t>(le_rom, collision_header_addr + 0x0C)) << std::endl;
    				std::cout << "N polys:      " << num_polys << std::endl;
    				for (int i = 0; i < num_polys; ++i) {
    					std::cout << "\t" << "Poly " << std::dec << i << std::endl;
    					std::cout << "\t\t" << "poly type: " << std::hex << be2le(read<uint16_t>(le_rom, poly_array_start + (16 * i))) << std::endl;
    					std::cout << "\t\t" << "{ " 
    							  << be2le(read<uint16_t>(le_rom, poly_array_start + (16 * i) + 0x02)) << ", " 
    							  << be2le(read<uint16_t>(le_rom, poly_array_start + (16 * i) + 0x04))<< ", " 
    							  << be2le(read<uint16_t>(le_rom, poly_array_start + (16 * i) + 0x06))<< " }" << std::endl;
    				}
    				std::cout << "N waterboxes: " << be2le(read<int16_t>(le_rom, collision_header_addr + 0x24)) << std::endl;
    				break;
    			}
    			case kRooms: {
    				std::cout << std::dec << (int)command.data1 << " rooms. " << std::endl;
    				uint64_t room_table_addr = seg2vrom(command.data2, scene_table_entries[FlagSceneToDump].scene_vrom_start);
    				for (int i = 0; i < command.data1; ++i) {
    					uint32_t room_start = be2le(read<uint32_t>(le_rom, room_table_addr + (i * 8)));
    					uint32_t room_end   = be2le(read<uint32_t>(le_rom, room_table_addr + 4 + (i * 8)));
    					z64_scene_command_t room_command;
    					uint64_t room_command_offset = 0;
    					do {
    						room_command = read_scene_command(le_rom, room_start + room_command_offset);
    						switch(room_command.code) {
    							case kRoomMesh:

    								break;
    						}
    						room_command_offset += sizeof(z64_scene_command_t);
    					} while(room_command.code != 0x14);
    				}
    				break;
    			}
    		}

    		scene_command_offset += sizeof(z64_scene_command_t);
    	} while(command.code != 0x14);
    }

    return 0;
}