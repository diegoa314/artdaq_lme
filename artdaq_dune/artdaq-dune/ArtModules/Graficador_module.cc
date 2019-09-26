
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core-dune/Overlays/FragmentType.hh"
#include "artdaq-core-dune/Overlays/PruebaFragmento.hh"
#include "artdaq-core-dune/Overlays/PruebaV2Fragmento.hh"

#include <cstdlib>
#include "TCanvas.h"
#include "TGraph.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

namespace prueba {

class Graficador : public art::EDAnalyzer {
public:
  explicit Graficador(fhicl::ParameterSet const & ps);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  Graficador(Graficador const &) = delete;
  Graficador(Graficador &&) = delete;
  Graficador & operator = (Graficador const &) = delete;
  Graficador & operator = (Graficador &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;
  void beginRun(art::Run const& run) override;

private:

  // Declare member data here.
  std::unique_ptr<TCanvas> canvas1;
  std::unique_ptr<TCanvas> canvas2;	
  std::unique_ptr<TGraph> grafico1;
  std::unique_ptr<TGraph> grafico2;
  int anchoCanvas; //ancho y largo de la ventana
  int largoCanvas;
  std::size_t datos_graph;//cantidad de puntos a graficar
  //std::string titulo_grafico;
  std::vector<double> x_vect;
  art::RunNumber_t run_t;

};
}

prueba::Graficador::Graficador(fhicl::ParameterSet const & ps):
	art::EDAnalyzer(ps),
	anchoCanvas(ps.get<int>("anchoCanvas",200)),
	largoCanvas(ps.get<int>("largoCanvas",200)),
	datos_graph(ps.get<size_t>("datos_graph",200))
{
	x_vect.resize(datos_graph);
	
	for (size_t i=0; i<datos_graph; i++) {
		x_vect[i]=(double)i;
	}
	
}	

void prueba::Graficador::beginRun(art::Run const& run) {
	std::cout<<"Grafico de datos"<<run.id()<<std::endl;	
	//este metodo se ejecuta cuando se da un start en el artdaq
	grafico1.reset(new TGraph(datos_graph)); /*se toma posesion 
	*del puntero construyendo el TGraph con la cantidad de puntos a graficar*/
	grafico1->SetTitle("Prueba Graficador");
	std::string name1="c1"; //nombre del canvas
	std::string title1="Grafico 1"; //titulo
	//funcion c_str() convierte un string a un puntero char
	canvas1.reset(new TCanvas(name1.c_str(),title1.c_str(),100,50,largoCanvas,anchoCanvas)); //se construye el objeto Canvas, los parametros 2 y 3 son su posicion en la ventana
	canvas1->SetGrid();		
	std::copy(x_vect.begin(),x_vect.end(),grafico1->GetX());
	//Se copia el vector de puntos x al vector de x del grafico
	
	grafico2.reset(new TGraph(datos_graph));
	grafico2->SetTitle("Prueba V2 Graficador");
	std::string name1="c2"; 
	std::string title1="Grafico 2";
	canvas2.reset(new TCanvas(name2.c_str(),title2.c_str(),600,550,largoCanvas,anchoCanvas));
	canvas2->SetGrid();		
	std::copy(x_vect.begin(),x_vect.end(),grafico1->GetX());

}

void prueba::Graficador::analyze(art::Event const& e) {
		
	artdaq::Fragments fragments;
	art::Handle<artdaq::Fragments> label_fragment; //un smart pointer
	e.getByLabel("daq","PRUEBA", "PRUEBAV2","ContainerPRUEBA","ContainerPRUEBAV2",label_fragment); //todos los fragmentos del evento con label daq o PRUEBA se guardan en label_fragment
	for(auto frag:*label_fragment){
		fragments.emplace_back(frag);//se guardan los punteros en fragments	
	}
	for(const auto& frag:fragments) {
		FragmentType frag_type=static_cast<FragmentType>(frag.type());
		switch (frag_type) {
		case FragmentType::PRUEBA:
			std::unique_ptr<prueba::PruebaFragmento> pruebafrag;
			pruebafrag.reset(new prueba::PruebaFragmento(frag));
			for (size_t i=0;i<datos_graph;i++) {
				uint8_t val=(uint8_t)pruebafrag->dataBeginADCs()[i];
				double tiempo=i; //modificar esto
				grafico1->SetPoint(i,tiempo,val);
			}			
			canvas1->cd(); //se incializa el canvas
			grafico1->Draw();
			canvas1->Modified();
			canvas1->Update();
			break;
		case FragmentType::PRUEBAV2:		
			std::unique_ptr<prueba::PruebaV2Fragmento> pruebafrag;
			pruebafrag.reset(new prueba::PruebaV2Fragmento(frag));
			for (size_t i=0;i<datos_graph;i++) {
				uint8_t val=(uint8_t)pruebafrag->dataBeginADCs()[i];
				double tiempo=i; //modificar esto
				grafico2->SetPoint(i,tiempo,val);
			}			
			canvas2->cd(); //se incializa el canvas
			grafico2->Draw();
			canvas2->Modified();
			canvas2->Update();
			break;
		}

		
	}
}
DEFINE_ART_MODULE(Graficador)






















