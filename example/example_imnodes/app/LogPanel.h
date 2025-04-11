#pragma once
#include <cstdarg>
#include <ctime>
#include <string>
#include <vector>
#include "imgui.h"

class ImGuiLogPanel {
public:
    ImGuiLogPanel( const char* name = "Log" )
        : _panelName( name )
        , _autoScroll( true )
        , _showTimestamp( true ) {}

    void Clear() {
        _buffer.clear();
    }

    void AddLog( const char* fmt, ... ) IM_FMTARGS( 2 ) {
        char buf[ 1024 ];
        va_list args;
        va_start( args, fmt );
        vsnprintf( buf, sizeof( buf ), fmt, args );
        va_end( args );

        if ( _showTimestamp ) {
            char timeBuf[ 64 ];
            std::time_t t = std::time( nullptr );
            std::strftime( timeBuf, sizeof( timeBuf ), "[%H:%M:%S] ", std::localtime( &t ) );
            _buffer.emplace_back( std::string( timeBuf ) + buf );
        }
        else {
            _buffer.emplace_back( buf );
        }
    }

    void Draw( float height = 200.0f ) {
        ImGui::Text( "%s", _panelName );
        ImGui::SameLine();
        if ( ImGui::SmallButton( "Clear" ) )
            Clear();
        ImGui::SameLine();
        ImGui::Checkbox( "Auto-scroll", &_autoScroll );
        ImGui::SameLine();
        ImGui::Checkbox( "Timestamp", &_showTimestamp );

        ImVec2 size = ImVec2( 0, height );
        ImGui::BeginChild( "LogRegion", size, true, ImGuiWindowFlags_HorizontalScrollbar );
        for ( const auto& line : _buffer )
            ImGui::TextUnformatted( line.c_str() );
        if ( _autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
            ImGui::SetScrollHereY( 1.0f );
        ImGui::EndChild();
    }

private:
    const char* _panelName;
    std::vector<std::string> _buffer;
    bool _autoScroll;
    bool _showTimestamp;
};
