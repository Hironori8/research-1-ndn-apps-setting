/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// custom-app.hpp

#ifndef CUSTOM_APP_H_
#define CUSTOM_APP_H_

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "src/ndnSIM/apps/ndn-producer.hpp"
#include "src/network/utils/packet-socket-factory.h"
#include "src/ndnSIM/ndn-cxx/src/security/signing-helpers.hpp"

namespace ns3 {
namespace ndn {
/**
 * @brief A simple custom application
 *
 * This applications demonstrates how to send Interests and respond with Datas to incoming interests
 *
 * When application starts it "sets interest filter" (install FIB entry) for /prefix/sub, as well as
 * sends Interest for this prefix
 *
 * When an Interest is received, it is replied with a Data with 1024-byte fake payload
 */
		class CustomProducer : public ndn::App {
		public:
				// register NS-3 type "CustomApp"
				static TypeId
				GetTypeId();
				CustomProducer();
				// (overridden from ndn::App) Processing upon start of the application
				virtual void
				StartApplication();

				// (overridden from ndn::App) Processing when application is stopped
				virtual void
				StopApplication();

				// (overridden from ndn::App) Callback that will be called when Interest arrives
				virtual void
				OnInterest(std::shared_ptr<const ndn::Interest> interest);

				// (overridden from ndn::App) Callback that will be called when Data arrives
				//virtual void
				//OnData(std::shared_ptr<const ndn::Data> contentObject);

		private:
				Name m_prefix;
				Name m_postfix;
				uint32_t m_virtualPayloadSize;
				Time m_freshness;
				Name m_identityName;
				uint32_t m_signature;
				Name m_keyLocator;
				ndn::KeyChain m_keyChain;
		//private:
				//void
				//SendInterest();
		};
}//ndn
} //ns3

#endif // CUSTOM_APP_H_
