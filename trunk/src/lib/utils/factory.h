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
 * File name: ./lib/utils/factory.h
 * First submitted: 2010-01-16
 * First submitter: Emiliano Leporati <emiliano.leporati@gmail.com>
 * Contributor(s) so far - 2010-07-30 :
 *     Emiliano Leporati <emiliano.leporati@gmail.com>
 *
 * Last changes :
 *     formats on their own; better Factory elements naming; separate library for every component
 *     Kinoglaz birth: pervasive bug fixing, improvements and client support
 *     sdp debugged
 *     interleave ok
 *     added licence disclaimer
 *
 **/


#ifndef __KGD_FACTORY_H
#define __KGD_FACTORY_H

#include <map>
#include <list>

#include "lib/common.h"
#include "lib/utils/map.hpp"
#include "lib/utils/virtual.hpp"
#include "lib/utils/pointer.hpp"
#include "lib/exceptions.h"

using namespace std;

namespace KGD
{
	//! from symbol to object - a static factory pattern to support serialization
	namespace Factory
	{
		// *********************************************************************************************************

		//! We must define a ClassID::type for every factory-enabled class
		namespace ClassID
		{
			typedef uint64_t type;
			const type INVALID = 0;
		}

		// *********************************************************************************************************

		//! this is the virtual base class for factory-enabled objects
		class Base
		: virtual public Virtual
		{
		};

		// *********************************************************************************************************

		//! Interface of a factory of objects of classes derived from AbstractType; AbstractType must extend Base class
		template< class AbstractType >
		class Abstract
		: public Virtual
		{
		public:
			//! should return a new instance of AbstractType
			virtual AbstractType newInstance() const = 0;
		};

		// *********************************************************************************************************

		//! A factory of objects of ConcreteType, who is derived from AbstractType
		template< class AbstractType, class ConcreteType >
		class Multi :
				public Abstract< AbstractType * >
		{
		public:
			//! returns new instance of ConcreteType using default constructor; ConcreteType is-a (must-be) AbstractType
			virtual AbstractType * newInstance() const;
		};

		// *********************************************************************************************************

		//! A "factory" of singleton instances of ConcreteType, who is derived from AbstractType
		template< class AbstractType, class ConcreteType >
		class Single :
				public Abstract< AbstractType & >
		{
		public:
			//! returns new instance of ConcreteType using default constructor; ConcreteType is-a (must-be) AbstractType
			virtual AbstractType & newInstance() const;
		};

		// *********************************************************************************************************

		//! this class is needed to provide compile-time static mappings for Enabled class types
		template< class AbstractType >
		class RegistrationHelper
		{
		private:
			ClassID::type _typeID;
		public:
			//! calls Registry< AbstractType >::registerFactory
			RegistrationHelper( ClassID::type typeID, const Abstract< AbstractType > * factory ) throw( Exception::InvalidState );
			//! calls Registry< AbstractType >::unregisterFactory
			~RegistrationHelper();
		};

		// *********************************************************************************************************

		//! storage for factory mappings
		extern map< string, void * > * GlobalMappings;

		//! global factory mappings

		//! mantains the static mapping (GlobalMappings) of class names
		//! (the template-instances of Registry) and their specific factory mappings
		//! this ensures cross-library unique entry points to factory instances, loaded
		//! into the (specific) registry during static initialization thanx to the
		//! RegistrationHelper instance referenced in Enabled constructor
		class Mappings
		{
		private:
		public:
			//! returns typed mappings associated to a correspondent registry instance
			template< class FactoryMap >
			static FactoryMap & getRegistryMappings( const string & className ) throw( Exception::NotFound );
			//! tells if this registry is registered
			static bool isRegistryRegistered( const string & className ) throw( );
			//! registers a new global registry mapping associated with a registry name
			static void registerRegistry( const string & className, void * mappings ) throw( Exception::InvalidState );
			//! unregisters the global registry mapping associated with given registry name
			template< class FactoryMap >
			static void unregisterRegistry( const string & className ) throw( Exception::NotFound );
		};

		//! This is the global registry: an interface to Mappings
		template< class AbstractType >
		class Registry
		{
		private:
			//! type of factory mappings in this tree
			typedef Ptr::Map< ClassID::type, const Abstract< AbstractType > > TFactoryMap;
		private:
			//! registers the bound between a typeID and an object factory
			static void registerFactory( ClassID::type typeID, const Abstract< AbstractType > * factory ) throw( Exception::InvalidState );
			//! registers the bound between a typeID and an object factory
			static void unregisterFactory( ClassID::type typeID ) throw( Exception::NotFound );
			//! just the helper can register factories
			friend class RegistrationHelper< AbstractType >;
		protected:
			//! returns internal class name to identify this registry in the GlobalMappings
			static string getClassName() throw();
			//! returns factory mappings for this factory tree
			static TFactoryMap & getFactoryMappings( ) throw( Exception::NotFound );
		public:
			virtual ~Registry();

			//! returns registered class ids
			static list< ClassID::type > getRegisteredClassIDs( ) throw();
			//! returns a reference to the factory for the given typeID
			static const Abstract< AbstractType > & getFactory( ClassID::type typeID ) throw( Exception::NotFound );

			//! returns a new instance using the factory bound to typeID
			static AbstractType newInstance( ClassID::type typeID ) throw( Exception::NotFound );

		};

		// *********************************************************************************************************

		//! class registry, for classes suitable to factory pattern
		template< class AbstractType >
		class ClassRegistry :
				public Registry< AbstractType * >
		{
		};

		template< class AbstractType >
		class InstanceRegistry :
				public Registry< AbstractType & >
		{
		};

		// *********************************************************************************************************

		//! A class should extend this template to be registered into Register< AbstractType > with typeID mapped to a default Class factory of ConcreteType
		template< class AbstractType, class FactoryClass, ClassID::type typeID >
		class Enabled :
			virtual public Base
		{
		private:
			static const RegistrationHelper< AbstractType > _entry;

		protected:
			//! statically allocates _entry to achieve compile-time registration
			Enabled();
		public:
			//! returns a reference to the registry holding the factory for this Enabled instance
			static const FactoryClass & getRegisteredFactory() throw();
			//! returns a reference to the registry holding the factory for this Enabled instance
			const FactoryClass & getFactory() const throw();
		};

		// *********************************************************************************************************

		template< class AbstractType, class ConcreteType, ClassID::type typeID >
		class Multiton :
			public Enabled< AbstractType *, Multi< AbstractType, ConcreteType >, typeID >
		{
		};

		template< class AbstractType, class ConcreteType, ClassID::type typeID >
		class Singleton :
			public Enabled< AbstractType &, Single< AbstractType, ConcreteType >, typeID >
		{
		};
	}
}

#endif

