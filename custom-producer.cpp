#include "custom-producer.hpp"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/random-variable-stream.h"
#include "ns3/ndnSIM/ndn-cxx/security/security-common.hpp"

#define IDENTITY_NAME "NDNSERVER"

NS_LOG_COMPONENT_DEFINE("CustomProducer");

namespace ns3 {
namespace ndn {
  
  NS_OBJECT_ENSURE_REGISTERED(CustomProducer);

  //$B%5!<%P$N(BID$BEPO?(B
TypeId
  CustomProducer::GetTypeId()
  {
    static TypeId tid = 
      TypeId("CustomProducer")
      .SetParent<ndn::App>()
      //$B?F$O(BApp$B%/%i%9(B
      .AddConstructor<CustomProducer>()
      .AddAttribute("Prefix", "Prefix, for which producer has the data", 
          StringValue("/"),
          MakeNameAccessor(&CustomProducer::m_prefix), 
          MakeNameChecker())
      .AddAttribute("Postfix",
          "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
          StringValue("/"), MakeNameAccessor(&CustomProducer::m_postfix), 
          MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", 
          UintegerValue(1024),
          MakeUintegerAccessor(&CustomProducer::m_virtualPayloadSize),
          MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
          TimeValue(Seconds(0)), MakeTimeAccessor(&CustomProducer::m_freshness),
          MakeTimeChecker())
      .AddAttribute(
          "Signature",
          "Fake signature, 0 valid signature (default), other values application-specific",
          UintegerValue(0), MakeUintegerAccessor(&CustomProducer::m_signature),
          MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&CustomProducer::m_keyLocator), 
  MakeNameChecker());
    return tid;
  }
  
  CustomProducer::CustomProducer()
    :m_identityName(IDENTITY_NAME)
  {
    NS_LOG_FUNCTION_NOARGS();
  }
  
  //$B%"%W%j%1!<%7%g%s$N%9%?!<%H(B
  void
  CustomProducer::StartApplication()
  {
    NS_LOG_FUNCTION_NOARGS();
    ndn::App::StartApplication();
    ndn::FibHelper::AddRoute(GetNode(),"/prefix", m_face,0);
    ndn::FibHelper::AddRoute(GetNode(),"/verification", m_face,0);
  }

  // Processing when application is stopped
  void
  CustomProducer::StopApplication()
  {
    NS_LOG_FUNCTION_NOARGS();
    // cleanup ndn::App
    ndn::App::StopApplication();
  }
  
  //$BMW5a%Q%1%C%H$,FO$$$?$N$A$KBP1~$9$k%G!<%?$rJV$9(B
  void
  CustomProducer::OnInterest(std::shared_ptr<const ndn::Interest> interest)
  {

    ndn::App::OnInterest(interest);
    if(!m_active)
      return;
    NS_LOG_DEBUG("Received Interest packet for " << interest->getName());

    // Note that Interests send out by the app will not be sent back to the app !

    auto data = std::make_shared<ndn::Data>(interest->getName());
    //$BMW5a%Q%1%C%H$NL>A0$HF1$8%G!<%?$r:n@.(B
    data->setName(interest->getName()); 
    data->setFreshnessPeriod(ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
    data->setContent(std::make_shared< ::ndn::Buffer>(m_virtualPayloadSize));
    
    //$B%3%s%F%s%D$rDI2C(B
    m_keyChain.createIdentity(m_identityName);
    m_keyChain.sign(*data, signingByIdentity(m_identityName));
    //$B%G!<%?$KG'>Z$rIU2C(B

    
    m_transmittedDatas(data, this, m_face);
    //$B%G!<%?$rAw$C$?$3$H$r<($9%H%l!<%9$rJV$9(B
    m_appLink->onReceiveData(*data);
    //$B%G!<%?$rG[Aw(B
    NS_LOG_DEBUG("Sending Data packet for " << data->getName());
    //$B%G!<%?$rAw$k$3$H$r%G%P%C%/$GJs9p(B
  }

}//ndn$B$NL>A06u4V$N=*$o$j(B
}//ns3$B$NL>A06u4V$N=*$o$j(B
