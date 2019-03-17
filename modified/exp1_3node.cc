#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h" //header files for simulation 
#include "ns3/pcap-file.h"
#include "ns3/gnuplot-helper.h"
#include "ns3/flow-monitor-module.h"
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");
void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet);

int main (int argc, char *argv[])
{
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

NodeContainer nodes;

	nodes.Create (3);

	

	InternetStackHelper stack;

	stack.Install (nodes);

	

	PointToPointHelper p2p1;

	p2p1.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));

	p2p1.SetChannelAttribute ("Delay", StringValue ("1ms"));



	PointToPointHelper p2p2;

	p2p2.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));

	p2p2.SetChannelAttribute ("Delay", StringValue ("5ms"));



	Ipv4AddressHelper address;

	address.SetBase ("101.1.1.0", "255.255.255.0"); 

	NetDeviceContainer devices;

	devices = p2p1.Install (nodes.Get (0), nodes.Get (1));

	Ipv4InterfaceContainer interfaces = address.Assign (devices);

	

	devices = p2p2.Install (nodes.Get (1), nodes.Get (2));

	address.SetBase ("101.1.3.0", "255.255.255.0"); 

	interfaces = address.Assign (devices);

	

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  

	UdpEchoServerHelper echoServer (9);



	ApplicationContainer serverApps = echoServer.Install (nodes.Get (2));

	serverApps.Start (Seconds (1.0));

	serverApps.Stop (Seconds (10.0));



	UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);

	echoClient.SetAttribute ("MaxPackets", UintegerValue (8));

	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1)));

	echoClient.SetAttribute ("PacketSize", UintegerValue (2048));



	ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));

	clientApps.Start (Seconds (1.0));

	clientApps.Stop (Seconds (20.0));


	AnimationInterface anim ("exp1.xml");

	anim.SetConstantPosition (nodes.Get(0), 100, 200);

        anim.SetConstantPosition (nodes.Get(1), 200, 150);

        anim.SetConstantPosition (nodes.Get(2), 300, 400);
         


        p2p1.EnablePcapAll("exp1_1");

        p2p2.EnablePcapAll("exp1_2");

	
//for data file
  //probeType = "ns3::Ipv4PacketProbe";
  //tracePath = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";
  GnuplotHelper plotHelper;
  plotHelper.ConfigurePlot ("UDP3_pkt",
                          "Packet Byte Count vs. Time",
                          "Time (Seconds)",
                          "Packet Byte Count",
                          "png");
  plotHelper.PlotProbe ("ns3::Ipv4PacketProbe",
                      "/NodeList/*/$ns3::Ipv4L3Protocol/Tx",
                      "OutputBytes",
                      "Packet Count",
                      GnuplotAggregator::KEY_BELOW);


 // Calculate Throughput using Flowmonitor
//Gnuplot parameters
      
    std::string fileNameWithNoExtension = "PKT3_UDP";
    std::string graphicsFileName        = fileNameWithNoExtension + ".png";
    std::string plotFileName            = fileNameWithNoExtension + ".plt";
    std::string plotTitle               = "Packet_Count vs Time";
    std::string dataTitle               = "PKT";

    // Instantiate the plot and set its title.
    Gnuplot gnuplot (graphicsFileName);
    gnuplot.SetTitle (plotTitle);

    // Make the graphics file, which the plot file will be when it
    // is used with Gnuplot, be a PNG file.
    gnuplot.SetTerminal ("png");

    // Set the labels for each axis.
    gnuplot.SetLegend ("Time", "Pkt_Count");

     
   Gnuplot2dDataset dataset;
   dataset.SetTitle (dataTitle);
   dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);



  //flowMonitor declaration
  FlowMonitorHelper fmHelper;
  Ptr<FlowMonitor> allMon = fmHelper.InstallAll();
  // call the flow monitor function
  ThroughputMonitor(&fmHelper, allMon, dataset); 


  // Simulator::Schedule (Seconds (50));
   Simulator::Stop(Seconds (10));
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
        //double localThrou=0;
		std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
		Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
		{
			Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
			std::cout<<"Flow ID			: " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
			std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
                        //double x=stats->second.txPackets;
			std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
                           double y=stats->second.rxPackets;
            std::cout<<"Duration		: "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;
			std::cout<<"Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
			//std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps"<<std::endl;
            //localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);
			// updata gnuplot data
            DataSet.Add((double)Simulator::Now().GetSeconds(),y);
			std::cout<<"---------------------------------------------------------------------------"<<std::endl;
		}
			Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon,DataSet);
   //if(flowToXml)
      {
	flowMon->SerializeToXmlFile ("ThroughputMonitor.xml", true, true);
      }

	}

