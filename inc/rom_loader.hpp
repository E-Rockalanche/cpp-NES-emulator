#ifndef ROM_LOADER_HPP
#define ROM_LOADER_HPP

#include <memory>

namespace nes
{

class Cartridge;

namespace Rom
{

std::unique_ptr<Cartridge> load( const char* filename );

}
}

#endif