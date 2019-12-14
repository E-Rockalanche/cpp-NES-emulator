#include "Apu.hpp"

#include "ByteIO.hpp"

#include "apu_snapshot.h"

#include <utility>

using namespace nes;

namespace
{
    const char* s_header = "APU";
}

Apu::Apu()
{
    constexpr int SampleRate = 48000;

    m_buffer.sample_rate( SampleRate );
    m_buffer.clock_rate( 1789773 );
    m_apu.output( &m_buffer );
    m_soundQueue.init( SampleRate );
}

void Apu::setMute( bool mute )
{
    m_muted = mute;
    m_apu.output( mute ? nullptr : &m_buffer );
}

void Apu::setDmcReader( dmc_reader_t func )
{
    m_apu.dmc_reader( std::move( func ) );
}

void Apu::reset()
{
    m_apu.reset();
    m_buffer.clear();
}

Byte Apu::read( cpu_time_t elapsedCycles, Word address )
{
    Byte value = 0;
    if ( address == m_apu.status_addr )
    {
        value = m_apu.read_status( elapsedCycles );
    }
    return value;
}

void Apu::write( cpu_time_t elapsedCycles, Word address, Byte value )
{
    m_apu.write_register( elapsedCycles, address, value );
}

void Apu::runFrame( cpu_time_t elapsedCycles )
{
    m_apu.end_frame( elapsedCycles );
    m_buffer.end_frame( elapsedCycles );

    if ( m_muted )
    {
        m_buffer.clear();
    }
    else if ( m_buffer.samples_avail() >= OutBufferSize )
    {
        size_t samples = m_buffer.read_samples( m_outBuf, OutBufferSize );
        m_soundQueue.write( m_outBuf, samples );
    }
}

void Apu::saveState( ByteIO::Writer& writer ) const
{
    writer.write( s_header );

    apu_snapshot_t apuState;
    m_apu.save_snapshot( &apuState );

    writer.write( apuState );
    writer.write( m_outBuf );
    writer.write( m_muted );
}

void Apu::loadState( ByteIO::Reader& reader )
{
    reader.readHeader( s_header );

    apu_snapshot_t apuState;

    reader.read( apuState );
    reader.read( m_outBuf );
    reader.read( m_muted );

    m_apu.load_snapshot( apuState );
}