/*************************************************************************
 *
 * Kinoglaz Streaming Server Daemon
 * Copyright (C) 2010 Emiliano Leporati ( emiliano.leporati@gmail.com )
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *************************************************************************
 *
 * File name: ./lib/utils/factory.hpp
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     termination, log, method execution refactor
 *     first import
 *
 **/

#ifndef __KGD_FACTORY_HPP
#define __KGD_FACTORY_HPP

#include "lib/utils/factory.h"
#include <lib/log.h>

namespace KGD
{
	namespace Factory
	{
		// ******************************************************************************************

		template< class AbstractType, class ConcreteType >
		AbstractType * Multi< AbstractType, ConcreteType >::newInstance() const
		{
			return new ConcreteType;
		}

		template< class AbstractType, class ConcreteType >
		AbstractType & Single< AbstractType, ConcreteType >::newInstance() const
		{
			return ConcreteType::getInstance();
		}

		// ******************************************************************************************

		template< class AbstractType >
		RegistrationHelper< AbstractType >::RegistrationHelper( ClassID::type typeID, const Abstract< AbstractType > * factory ) throw( Exception::InvalidState )
		: _typeID( typeID )
		{
			// do register the binding
			Registry< AbstractType >::registerFactory( typeID, factory );
		}

		template< class AbstractType >
		RegistrationHelper< AbstractType >::~RegistrationHelper()
		{
			Registry< AbstractType >::unregisterFactory( _typeID );
		}

		// ******************************************************************************************

		template< class FactoryMap >
		FactoryMap & Mappings::getRegistryMappings( const string & className ) throw( Exception::NotFound )
		{
			if( GlobalMappings && GlobalMappings->find( className ) != GlobalMappings->end() )
			{
				return *( reinterpret_cast< FactoryMap * >( (*GlobalMappings)[ className ] ) );
			}
			else
				throw Exception::NotFound( "factory mappings for tree " + className );
		}

		template< class FactoryMap >
		void Mappings::unregisterRegistry( const string & className ) throw( Exception::NotFound )
		{
			if( GlobalMappings && GlobalMappings->find( className ) != GlobalMappings->end() )
			{
				Log::debug( "removing factory mapping for tree %s", className.c_str() );
				FactoryMap * f = reinterpret_cast< FactoryMap * >( (*GlobalMappings)[ className ] );
				Ptr::clear( f );
				GlobalMappings->erase( className );
				if ( GlobalMappings->empty() )
				{
					Log::debug( "removing global mapping" );
					Ptr::clear( GlobalMappings );
				}
			}
			else
				throw Exception::NotFound( "factory mappings for tree " + className );
		}

		// ******************************************************************************************

		template< class AbstractType >
		Registry< AbstractType >::~Registry()
		{
		}

		template< class AbstractType > string Registry< AbstractType >::getClassName() throw()
		{
			return typeid( Registry< AbstractType > ).name();
		}

		template< class AbstractType >
		typename Registry< AbstractType >::TFactoryMap & Registry< AbstractType >::getFactoryMappings( ) throw( Exception::NotFound )
		{
			return Mappings::getRegistryMappings< TFactoryMap >( getClassName() );
		}

		template< class AbstractType >
		void Registry< AbstractType >::registerFactory( ClassID::type typeID, const Abstract< AbstractType > * factory ) throw( Exception::InvalidState )
		{
			Log::debug( "registering class id %llu", typeID );

			// setup local mappings
			string className = getClassName();
			if ( ! Mappings::isRegistryRegistered( className ) )
				Mappings::registerRegistry( className, new TFactoryMap );

			// add type id to local mappings
			TFactoryMap & f = Registry< AbstractType >::getFactoryMappings( );
			if ( ! f.has( typeID ) )
				f( typeID ) = factory;
			else
				throw Exception::InvalidState("already registered factory specification for typeID " + toString( typeID ));
		}

		template< class AbstractType >
		void Registry< AbstractType >::unregisterFactory( ClassID::type typeID ) throw( Exception::NotFound )
		{
			string className = getClassName();
			if ( ! Mappings::isRegistryRegistered( className ) )
				throw Exception::NotFound("registry empty during unregistration of " + toString( typeID ));

			// remove type id from local mappings
			TFactoryMap & f = Registry< AbstractType >::getFactoryMappings( );
			if ( f.has( typeID ) )
			{
				Log::debug( "unregistering class id %llu", typeID );

				f.erase( typeID );

				if ( f.empty() )
					Mappings::unregisterRegistry< TFactoryMap >( className );
			}
			else
				throw Exception::NotFound("no factory specification for typeID " + toString( typeID ));
		}

		template< class AbstractType >
		list< ClassID::type > Registry< AbstractType >::getRegisteredClassIDs() throw()
		{
			TFactoryMap & f = Registry< AbstractType >::getFactoryMappings( );
			return f.keys();
		}

		template< class AbstractType >
		AbstractType Registry< AbstractType >::newInstance ( ClassID::type typeID ) throw( Exception::NotFound )
		{
			TFactoryMap & f = Registry< AbstractType >::getFactoryMappings( );
			const Abstract< AbstractType > & fac = f( typeID, Exception::NotFound( "factory for typeID " + toString( typeID ) ) );
			return fac.newInstance();
		}

		template< class AbstractType >
		const Abstract< AbstractType > & Registry< AbstractType >::getFactory( ClassID::type typeID ) throw( Exception::NotFound )
		{
			TFactoryMap & f = Registry< AbstractType >::getFactoryMappings( );
			return f( typeID, Exception::NotFound( "factory for typeID " + toString( typeID ) ) );
		}

		// ******************************************************************************************

		template< class AbstractType, class FactoryClass, ClassID::type typeID >
		Enabled< AbstractType, FactoryClass, typeID >::Enabled()
		{
			// without this, compiler will cut declaration to optimize code
			const RegistrationHelper< AbstractType > & mustBeHere __attribute__((unused)) = _entry ;
		}

		template< class AbstractType, class FactoryClass, ClassID::type typeID >
		const FactoryClass &
		Enabled< AbstractType, FactoryClass, typeID >::getRegisteredFactory() throw()
		{
			return Registry< AbstractType >::getFactory( typeID ).Virtual::as< FactoryClass >();
		}

		template< class AbstractType, class FactoryClass, ClassID::type typeID >
		const FactoryClass &
		Enabled< AbstractType, FactoryClass, typeID >::getFactory() const throw()
		{
			return getRegisteredFactory();
		}

		template< class AbstractType, class FactoryClass, ClassID::type typeID >
		const RegistrationHelper< AbstractType >
		Enabled< AbstractType, FactoryClass, typeID >::_entry = RegistrationHelper< AbstractType > ( typeID, new FactoryClass() );

	}
}

#endif
