// --------------------------------------------------------------------------------------------------
//  Copyright (c) 2016 Microsoft Corporation
//  
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
//  associated documentation files (the "Software"), to deal in the Software without restriction,
//  including without limitation the rights to use, copy, modify, merge, publish, distribute,
//  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//  
//  The above copyright notice and this permission notice shall be included in all copies or
//  substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// --------------------------------------------------------------------------------------------------

// Local:
#include "ClientConnection.h"

// Boost:
#include <boost/bind.hpp>

namespace malmo
{
    boost::shared_ptr< ClientConnection > ClientConnection::create( boost::asio::io_service& io_service, std::string address, int port )
    {
        return boost::shared_ptr< ClientConnection >(new ClientConnection( io_service, address, port ) );
    }
    
    void ClientConnection::send( std::string message )
    {
        this->io_service.post( boost::bind( &ClientConnection::writeImpl, shared_from_this(), message ) );
    }

    ClientConnection::ClientConnection( boost::asio::io_service& io_service, std::string address, int port )
        : io_service( io_service )
    {
        // connect the socket to the requested endpoint
        boost::asio::ip::tcp::resolver resolver( io_service );
        boost::asio::ip::tcp::resolver::query query( address, std::to_string( port ) );
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve( query );
        this->socket = std::unique_ptr< boost::asio::ip::tcp::socket >( new boost::asio::ip::tcp::socket( io_service ) );
        boost::asio::connect( *this->socket, endpoint_iterator );            
    }
    
    ClientConnection::~ClientConnection()
    {        
    }
        
    void ClientConnection::writeImpl( std::string message )
    {
        boost::lock_guard<boost::mutex> scope_guard(this->outbox_mutex);

        if (message.back() != '\n')
            message += '\n';
        this->outbox.push_back( message );
        if ( this->outbox.size() > 1 ) {
            // outstanding write
            return;
        }

        this->write();
    }
    
    void ClientConnection::write()
    {
        const std::string& message = this->outbox.front();
        boost::asio::async_write(
            *this->socket,
            boost::asio::buffer( message ),
            boost::bind(
                &ClientConnection::wrote,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred
            ) 
        );
    }

    void ClientConnection::wrote( const boost::system::error_code& error,
                                  size_t bytes_transferred )
    {        
        boost::lock_guard<boost::mutex> scope_guard(this->outbox_mutex);

        this->outbox.pop_front();
        if( !this->outbox.empty() )
            this->write();  
    }
}
