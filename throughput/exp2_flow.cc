	/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/gnuplot-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SixthScriptExample");

// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms
//
//
// We want to look at changes in the ns-3 TCP congestion window.  We need
// to crank up a flow and hook the CongestionWindow attribute on the socket
// of the sender.  Normally one would use an on-off application to generate a
// flow, but this has a couple of problems.  First, the socket of the on-off
// application is not created until Application Start time, so we wouldn't be
// able to hook the socket (now) at configuration time.  Second, even if we
// could arrange a call after start time, the socket is not public so we
// couldn't get at it.
//
// So, we can cook up a simple version of the on-off application that does what
// we want.  On the plus side we don't need all of the complexity of the on-off
// application.  On the minus side, we don't have a helper, so we have to get
// a little more involved in the details, but this is trivial.
//
// So first, we create a socket and do the trace connect on it; then we pass
// this socket into the constructor of our simple application which we then
// install in the source node.
// ===========================================================================
//
class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private: 
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}
MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet);

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  NodeContainer nodes;
  nodes.Create (3);
  
  InternetStackHelper stack;
  stack.Install (nodes);

  PointToPointHelper p2p1;
  p2p1.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p1.SetChannelAttribute ("Delay", StringValue ("4ms"));
  
  PointToPointHelper p2p2;
  p2p2.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p2.SetChannelAttribute ("Delay", StringValue ("4ms"));
  
  Ipv4AddressHelper address;
  address.SetBase ("20.1.1.0", "255.255.255.252");
  NetDeviceContainer devices;
  devices = p2p1.Install (nodes.Get(0),nodes.Get(1));
  Ipv4InterfaceContainer interfaces = address.Assign (devices);
  
  NetDeviceContainer devices1;
  devices1 = p2p2.Install (nodes.Get(2),nodes.Get(1));
  address.SetBase ("20.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces1 = address.Assign (devices1);
  
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Ptr<RateErrorModel> em1 = CreateObject<RateErrorModel> ();
  em1->SetAttribute ("ErrorRate", DoubleValue (0.0000001));
  devices1.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));
  
 
  uint16_t sinkPort = 8080;
  Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (100.));
  
  uint16_t sinkPort1 = 8090;
  Address sinkAddress1 (InetSocketAddress (interfaces1.GetAddress (1), sinkPort1));
  PacketSinkHelper packetSinkHelper1 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort1));
  ApplicationContainer sinkApps1 = packetSinkHelper1.Install (nodes.Get (1));
  sinkApps1.Start (Seconds (0.));
  sinkApps1.Stop (Seconds (100.));

  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000, DataRate ("5Mbps"));
  nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (100.));
  
  Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (nodes.Get (2), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> app1 = CreateObject<MyApp> ();
  app1->Setup (ns3TcpSocket1, sinkAddress1, 1040, 1000, DataRate ("10Mbps"));
  nodes.Get (2)->AddApplication (app1);
  app1->SetStartTime (Seconds (1.));
  app1->SetStopTime (Seconds (100.));

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("exp2_1.cwnd");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
  Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream ("exp2_2.cwnd");
  ns3TcpSocket1->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream1));

  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("exp2.pcap", std::ios::out, PcapHelper::DLT_PPP);
  //devices1.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));
  devices1.Get (0)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));



//for data file
  //probeType = "ns3::Ipv4PacketProbe";
  //tracePath = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";
  GnuplotHelper plotHelper;
  plotHelper.ConfigurePlot ("exp2",
                          "Packet Byte Count vs. Time",
                          "Time (Seconds)",
                          "Packet Byte Count",
                          "png");
  plotHelper.PlotProbe ("ns3::Ipv4PacketProbe",
                      "/NodeList/*/$ns3::Ipv4L3Protocol/Tx",
                      "OutputBytes",
                      "Packet Byte Count",
                      GnuplotAggregator::KEY_BELOW);
                      
  p2p1.EnablePcapAll("exp2_1");
  p2p2.EnablePcapAll("exp2_2");
  
  
  // Calculate Throughput using Flowmonitor
//Gnuplot parameters
      
    std::string fileNameWithNoExtension = "Throughput_exp2";
    std::string graphicsFileName        = fileNameWithNoExtension + ".png";
    std::string plotFileName            = fileNameWithNoExtension + ".plt";
    std::string plotTitle               = "Flow vs Throughput";
    std::string dataTitle               = "Throughput";

    // Instantiate the plot and set its title.
    Gnuplot gnuplot (graphicsFileName);
    gnuplot.SetTitle (plotTitle);

    // Make the graphics file, which the plot file will be when it
    // is used with Gnuplot, be a PNG file.
    gnuplot.SetTerminal ("png");

    // Set the labels for each axis.
    gnuplot.SetLegend ("Flow", "Throughput");

     
   Gnuplot2dDataset dataset;
   dataset.SetTitle (dataTitle);
   dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);



  //flowMonitor declaration
  FlowMonitorHelper fmHelper;
  Ptr<FlowMonitor> allMon = fmHelper.InstallAll();
  // call the flow monitor function
  ThroughputMonitor(&fmHelper, allMon, dataset); 


  // Simulator::Schedule (Seconds (50));
   Simulator::Stop(Seconds (40));
Simulator::Run ();

//Gnuplot ...continued
  gnuplot.AddDataset (dataset);
  // Open the plot file.
  std::ofstream plotFile (plotFileName.c_str());
  // Write the plot file.
  gnuplot.GenerateOutput (plotFile);
  // Close the plot file.
  plotFile.close ();










  Simulator::Destroy ();

  return 0;
}

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
	{
        double localThrou=0;
		std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
		Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
		{
			Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
			std::cout<<"Flow ID			: " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
			std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
			std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
            std::cout<<"Duration		: "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;
			std::cout<<"Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
			std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps"<<std::endl;
            localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);
			// updata gnuplot data
            DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
			std::cout<<"---------------------------------------------------------------------------"<<std::endl;
		}
			Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon,DataSet);
   //if(flowToXml)
      {
	flowMon->SerializeToXmlFile ("ThroughputMonitor.xml", true, true);
      }

	}

