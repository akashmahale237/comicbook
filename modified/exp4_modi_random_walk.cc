/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
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
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot-helper.h"
#include "ns3/nstime.h"
//#include "random-waypoint-mobility-model.h"

using namespace ns3;



NS_LOG_COMPONENT_DEFINE ("LenaX2HandoverExample");

void
NotifyConnectionEstablishedUe (std::string context,
                               uint64_t imsi,
                               uint16_t cellid,
                               uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": connected to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": previously connected to CellId " << cellid
            << " with RNTI " << rnti
            << ", doing handover to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": successful handover to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyConnectionEstablishedEnb (std::string context,
                                uint64_t imsi,
                                uint16_t cellid,
                                uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": successful connection of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti,
                        uint16_t targetCellId)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": start handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << " to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": completed handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet);
/**
 * Sample simulation script for a X2-based handover.
 * It instantiates two eNodeB, attaches one UE to the 'source' eNB and
 * triggers a handover of the UE towards the 'target' eNB.
 */

  // --------------------------------------------------------------------------------------------
// static void 
// CourseChange (std::string foo, Ptr<const MobilityModel> mobility)
// {
//   Vector pos = mobility->GetPosition ();
//   Vector vel = mobility->GetVelocity ();
//   std::cout << Simulator::Now () << ", model=" << mobility << ", POS: x=" << pos.x << ", y=" << pos.y
//             << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
//             << ", z=" << vel.z << std::endl;
// }
  // --------------------------------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  // LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);

  // LogComponentEnable ("LteHelper", logLevel);
  // LogComponentEnable ("EpcHelper", logLevel);
  // LogComponentEnable ("EpcEnbApplication", logLevel);
  // LogComponentEnable ("EpcX2", logLevel);
  // LogComponentEnable ("EpcSgwPgwApplication", logLevel);

  // LogComponentEnable ("LteEnbRrc", logLevel);
  // LogComponentEnable ("LteEnbNetDevice", logLevel);
  // LogComponentEnable ("LteUeRrc", logLevel);
  // LogComponentEnable ("LteUeNetDevice", logLevel);

  uint16_t numberOfUes = 8;
  uint16_t numberOfEnbs = 2;
  uint16_t numBearersPerUe = 0;
  //double simTime = 0.300;
  double distance = 900.0;
  double yForUe = 500.0;
  double speed = 100;
  double simTime = (double)(numberOfEnbs + 1) * distance / speed;
  double enbTxPowerDbm = 46.0;

  // change some default attributes so that they are reasonable for
  // this scenario, but do this before processing command line
  // arguments, so that the user is allowed to override these settings
  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (1000000));
  Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (false));

  // Command line arguments
  CommandLine cmd;
  //cmd.AddValue ("numberOfUes", "Number of UEs", numberOfUes);
  //cmd.AddValue ("numberOfEnbs", "Number of eNodeBs", numberOfEnbs);
  cmd.AddValue ("simTime", "Total duration of the simulation (in seconds)", simTime);
  cmd.AddValue ("speed", "Speed of the UE (default = 20 m/s)", speed);
  cmd.AddValue ("enbTxPowerDbm", "TX power [dBm] used by HeNBs (defalut = 46.0)", enbTxPowerDbm);
  cmd.Parse (argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
//Fading Model

 lteHelper->SetAttribute ("FadingModel", StringValue ("ns3::TraceFadingLossModel"));
  
  std::ifstream ifTraceFile;
  ifTraceFile.open ("../../src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad", std::ifstream::in);
  if (ifTraceFile.good ())
   {
      //script launched by test.py
      lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue ("../../src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"));
    }
 else
    {
     // script launched as an example
     lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue ("src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"));
    }

//fading model ends

  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

  lteHelper->SetHandoverAlgorithmType ("ns3::A2A4RsrqHandoverAlgorithm");
  lteHelper->SetHandoverAlgorithmAttribute ("ServingCellThreshold",
                                            UintegerValue (30));
  lteHelper->SetHandoverAlgorithmAttribute ("NeighbourCellOffset",
                                            UintegerValue (1));
  lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm");
  lteHelper->SetHandoverAlgorithmAttribute ("Hysteresis",
                                             DoubleValue (3.0));
  lteHelper->SetHandoverAlgorithmAttribute ("TimeToTrigger",
                                           TimeValue (MilliSeconds (256)));

  //lteHelper->SetHandoverAlgorithmType ("ns3::NoOpHandoverAlgorithm"); // disable automatic handover

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);


  // Routing of the Internet Host (towards the LTE network)
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  // interface 0 is localhost, 1 is the p2p device
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (numberOfEnbs);
  ueNodes.Create (numberOfUes);

    // Install Mobility Model in eNB
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfEnbs; i++)
    {
      Vector enbPosition (distance * (3*i), distance+300, 0);
      enbPositionAlloc->Add (enbPosition);
    }
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (enbPositionAlloc);
  enbMobility.Install (enbNodes);

   // Install Mobility Model in UE Constant Acceleration
  MobilityHelper ueMobility;
  ueMobility.SetMobilityModel ("ns3::ConstantAccelerationMobilityModel");
  ueMobility.Install (ueNodes);
  ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0, yForUe+50, 0));
  ueNodes.Get (0)->GetObject<ConstantAccelerationMobilityModel> ()->SetVelocityAndAcceleration (Vector (speed, 0, 0), Vector (1,0,0));
  ueNodes.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (500, 500, 500));
  ueNodes.Get (1)->GetObject<ConstantAccelerationMobilityModel> ()->SetVelocityAndAcceleration (Vector (-1*speed,0.5*speed,0), Vector (1,0,0));
  ueNodes.Get (2)->GetObject<MobilityModel> ()->SetPosition (Vector (700, 600, 600));
  ueNodes.Get (2)->GetObject<ConstantAccelerationMobilityModel> ()->SetVelocityAndAcceleration (Vector (-1*speed,0.1*speed,0), Vector (1,0,0));
  ueNodes.Get (3)->GetObject<MobilityModel> ()->SetPosition (Vector (900, 700, 700));
  ueNodes.Get (3)->GetObject<ConstantAccelerationMobilityModel> ()->SetVelocityAndAcceleration (Vector (-1*speed,0.2*speed,0), Vector (1,0,0));
  ueNodes.Get (4)->GetObject<MobilityModel> ()->SetPosition (Vector (1100, 800, 800));
  ueNodes.Get (4)->GetObject<ConstantAccelerationMobilityModel> ()->SetVelocityAndAcceleration (Vector (-1*speed,0.3*speed,0), Vector (1,0,0));
  ueNodes.Get (5)->GetObject<MobilityModel> ()->SetPosition (Vector (1300, 900, 900));
  ueNodes.Get (5)->GetObject<ConstantAccelerationMobilityModel> ()->SetVelocityAndAcceleration (Vector (-1*speed,0.4*speed,0), Vector (1,0,0));
  ueNodes.Get (6)->GetObject<MobilityModel> ()->SetPosition (Vector (1500, 1000, 1000));
  ueNodes.Get (6)->GetObject<ConstantAccelerationMobilityModel> ()->SetVelocityAndAcceleration (Vector (-1*speed,0.4*speed,0), Vector (1,0,0));
  ueNodes.Get (7)->GetObject<MobilityModel> ()->SetPosition (Vector (1700, 1100, 1100));
  ueNodes.Get (7)->GetObject<ConstantAccelerationMobilityModel> ()->SetVelocityAndAcceleration (Vector (-1*speed,0.4*speed,0), Vector (1,0,0));


  // Install Mobility Model Constant Position
  /* 
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfEnbs; i++)
    {
      positionAlloc->Add (Vector (distance * 2 * i - distance, 0, 0));
    }
  for (uint16_t i = 0; i < numberOfUes; i++)
    {
      positionAlloc->Add (Vector (0, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (enbNodes);
  mobility.Install (ueNodes);*/


  // --------------------------------------------------------------------------------------------
  // To Install Mobility Model Random Walk UNCOMMENT: following snipet and above function named 'CourseChange' and COMMENT variable declaration for 'yForUe'
  
  // Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Mode", StringValue ("Time"));
  // Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Time", StringValue ("10s"));
  // Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  // Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Bounds", StringValue ("0|200|0|200"));

  // cmd.Parse (argc, argv);
 
  // MobilityHelper uemobility;

  // uemobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
  //                                "X", StringValue ("100.0"),
  //                                "Y", StringValue ("100.0"),
  //                                "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));

  // uemobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //                            "Mode", StringValue ("Time"),
  //                            "Time", StringValue ("10s"),
  //                            "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
  //                            "Bounds", StringValue ("0|200|0|200"));
  // uemobility.Install (ueNodes);
  // Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
  //                  MakeCallback (&CourseChange));
  // --------------------------------------------------------------------------------------------





  // Install LTE Devices in eNB and UEs
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));


  // Attach all UEs to the first eNodeB
  /*for (uint16_t i = 0; i < numberOfUes; i++)
    {
      lteHelper->Attach (ueLteDevs.Get (i), enbLteDevs.Get (0));
    }*/
  lteHelper->Attach (ueLteDevs.Get(0), enbLteDevs.Get(0));
  lteHelper->Attach (ueLteDevs.Get(1), enbLteDevs.Get(0));
  lteHelper->Attach (ueLteDevs.Get(2), enbLteDevs.Get(0));
  lteHelper->Attach (ueLteDevs.Get(3), enbLteDevs.Get(0));
  lteHelper->Attach (ueLteDevs.Get(4), enbLteDevs.Get(1));
  lteHelper->Attach (ueLteDevs.Get(5), enbLteDevs.Get(1));
  lteHelper->Attach (ueLteDevs.Get(6), enbLteDevs.Get(1));
  lteHelper->Attach (ueLteDevs.Get(7), enbLteDevs.Get(1));

      
  NS_LOG_LOGIC ("setting up applications");

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 10000;
  uint16_t ulPort = 20000;

  // randomize a bit start times to avoid simulation artifacts
  // (e.g., buffer overflows due to packet transmissions happening
  // exactly at the same time)
  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.010));

  for (uint32_t u = 0; u < numberOfUes; ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
          ++dlPort;
          ++ulPort;

          ApplicationContainer clientApps;
          ApplicationContainer serverApps;

          NS_LOG_LOGIC ("installing UDP DL app for UE " << u);
          UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
          clientApps.Add (dlClientHelper.Install (remoteHost));
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), dlPort));
          serverApps.Add (dlPacketSinkHelper.Install (ue));

          NS_LOG_LOGIC ("installing UDP UL app for UE " << u);
          UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
          clientApps.Add (ulClientHelper.Install (ue));
          PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), ulPort));
          serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

          Ptr<EpcTft> tft = Create<EpcTft> ();
          EpcTft::PacketFilter dlpf;
          dlpf.localPortStart = dlPort;
          dlpf.localPortEnd = dlPort;
          tft->Add (dlpf);
          EpcTft::PacketFilter ulpf;
          ulpf.remotePortStart = ulPort;
          ulpf.remotePortEnd = ulPort;
          tft->Add (ulpf);
          EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
          lteHelper->ActivateDedicatedEpsBearer (ueLteDevs.Get (u), bearer, tft);

          Time startTime = Seconds (startTimeSeconds->GetValue ());
          serverApps.Start (startTime);
          clientApps.Start (startTime);

        } // end for b
    }


  // Add X2 inteface
  lteHelper->AddX2Interface (enbNodes);

  // X2-based Handover
  lteHelper->HandoverRequest (Seconds (0.100), ueLteDevs.Get (0), enbLteDevs.Get (0), enbLteDevs.Get (1));

  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-x2-handover");

  lteHelper->EnablePhyTraces ();
  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  lteHelper->EnablePdcpTraces ();
  Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
  rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (1.0)));
  rlcStats-> SetAttribute ("DlRlcOutputFilename", StringValue ("DLRLCexp4withchannel.txt"));
  rlcStats-> SetAttribute ("UlRlcOutputFilename", StringValue ("ULRLCexp4withchannel.txt"));
  Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats ();
  pdcpStats->SetAttribute ("EpochDuration", TimeValue (Seconds (1.0)));


  // connect custom trace sinks for RRC connection establishment and handover notification
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkUe));

  AnimationInterface anim("exp4.xml");
 
  // Calculate Throughput using Flowmonitor
//Gnuplot parameters
      
    std::string fileNameWithNoExtension = "Throughput_exp4";
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


