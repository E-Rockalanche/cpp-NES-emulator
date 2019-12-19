#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include "BankMapper.hpp"
#include "ByteIO.hpp"
#include "Memory.hpp"
#include "types.hpp"

namespace nes
{

	class Cpu;
	
	class Cartridge
	{
	public:

		enum class NameTableMirroring
		{
			Horizontal,
			/*
			       000 400
			0x2000 A   A
			0x2800 B   B
			*/

			Vertical
			/*
			       000 400
			0x2000 A   B
			0x2800 A   B
			*/
		};

		Cartridge( Memory data );

		virtual ~Cartridge() = default;

		Byte readPRG( Word address );
		Byte readCHR( Word address );
		
		virtual void writePRG( Word address, Byte value );
		virtual void writeCHR( Word address, Byte value );

		virtual void signalScanline() {}
		virtual void setCPU( Cpu& ) {}

		virtual void reset();

		virtual void saveState( ByteIO::Writer& writer );
		virtual void loadState( ByteIO::Reader& reader );

		virtual const char* getName() const { return "NROM"; }

		bool saveGame( const char* filename );
		bool loadSave( const char* filename );

		bool hasSRAM() const;

		NameTableMirroring getNameTableMirroring() const { return m_mirroring; }

		uint32_t getChecksum() const { return m_checksum; }

	protected:

		Byte* getPrg() { return m_prg; }
		size_t getPrgSize() const { return m_prgSize; }

		Byte* getChr() { return m_chr; }
		size_t getChrSize() const { return m_chrSize; }

		Byte* getRam() { return m_ram.data(); }
		size_t getRamSize() const { return m_ram.size(); }

		void setNameTableMirroring( NameTableMirroring mirroring )
		{
			m_mirroring = mirroring;
		}

		void setPrgBank( size_t slot, int bank, size_t bankSize )
		{
			m_prgMap.setBank( slot, bank, bankSize );
		}

		void setChrBank( size_t slot, int bank, size_t bankSize )
		{
			m_chrMap.setBank( slot, bank, bankSize );
		}

		static constexpr Word CartridgeStart = 0x4020;
		static constexpr Word RamStart = 0x6000;
		static constexpr Word PrgStart = 0x8000;

		static constexpr size_t NumPrgSlots = 4;
		static constexpr size_t PrgBankSize = 0x2000;

		static constexpr size_t NumChrSlots = 8;
		static constexpr size_t ChrBankSize = 0x0400;

	private:

		Memory m_data;
		Memory m_ram;
		Memory m_chrRam;

		Byte* m_prg = nullptr;
		size_t m_prgSize = 0;

		Byte* m_chr = nullptr;
		size_t m_chrSize = 0;

		NameTableMirroring m_mirroring = NameTableMirroring::Horizontal;

		BankMapper<NumPrgSlots, PrgBankSize> m_prgMap;
		BankMapper<NumChrSlots, ChrBankSize> m_chrMap;

		uint32_t m_checksum = 0;
	};

}

#endif