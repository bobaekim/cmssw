
#include "TVector3.h"
#include "TGLUtil.h"
#include <TH2.h>
#include <TBox.h>
#include <TLine.h>
#include <TLatex.h>
#include <TPaveText.h>
#include <TCanvas.h>
#include <TEveWindow.h>
#include "TRootEmbeddedCanvas.h"

#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/TrackReco/interface/HitPattern.h"
#include "DataFormats/TrackingRecHit/interface/TrackingRecHit.h"

#include "Fireworks/Core/interface/FWDetailView.h"
#include "Fireworks/Core/interface/DetIdToMatrix.h"
#include "Fireworks/Core/interface/FWEventItem.h"
#include "Fireworks/Core/interface/FWModelId.h"
#include "Fireworks/Core/interface/FWDetailView.h"
#include "Fireworks/Tracks/plugins/FWTrackResidualDetailView.h"

using reco::Track;
using reco::TrackBase;
using reco::HitPattern;
using reco::TrackResiduals;

const char* FWTrackResidualDetailView::m_det_tracker_str[6]={"PXB","PXF","TIB","TID","TOB","TEC"};

FWTrackResidualDetailView::FWTrackResidualDetailView ():
   m_ndet(0),
   m_nhits(0),
   m_resXFill(3007),
   m_resXCol(kGreen-9),
   m_resYFill(3006),
   m_resYCol(kWhite),
   m_stereoFill(3004),
   m_stereoCol(kCyan-9),
   m_invalidFill(3001),
   m_invalidCol(kRed)
{
}

FWTrackResidualDetailView::~FWTrackResidualDetailView ()
{
    getEveWindow()->DestroyWindow();
}


void
FWTrackResidualDetailView::prepareData(const FWModelId &id, const reco::Track* track)
{
   HitPattern hitpat = track->hitPattern();
   TrackResiduals residuals = track->residuals();

   const DetIdToMatrix *detIdToGeo = id.item()->getGeom();
   assert(detIdToGeo != 0);
   m_nhits=hitpat.numberOfHits();
   for (int i = 0; i < m_nhits; ++i) {
      //   	printf("there are %d hits in the pattern, %d in the vector, this is %u\n",
      //   	       m_nhits, track->recHitsEnd() - track->recHitsBegin(), (*(track->recHitsBegin() + i))->geographicalId().rawId());
      hittype[i] = 0x3 & hitpat.getHitPattern(i);
      stereo[i] = 0x1 & hitpat.getHitPattern(i) >> 2;
      subsubstruct[i] = 0xf & hitpat.getHitPattern(i) >> 3;
      substruct[i] = 0x7 & hitpat.getHitPattern(i) >> 7;
      m_detector[i] = 0x01 & hitpat.getHitPattern(i) >> 10;
      if ((*(track->recHitsBegin() + i))->isValid()) {
         res[0][i] = getSignedResidual(detIdToGeo,
                                       (*(track->recHitsBegin() + i))->geographicalId().rawId(),
                                       residuals.residualX(i, hitpat));
      } else {
         res[0][i] = 0;
      }
      res[1][i] = residuals.residualY(i, hitpat);
      // printf("%s, %i\n",m_det_tracker_str[substruct[i]-1],subsubstruct[i]);
   }

   m_det[0]=0;
   for(int j=0; j < m_nhits-1;) {
      int k=j+1;
      for(; k<m_nhits ; k++) {
         if(substruct[j]==substruct[k]  && subsubstruct[j]==subsubstruct[k]) {
            if(k==(m_nhits-1)) j=k;
         }
         else {
            m_ndet++;
            j=k;
            m_det[m_ndet]=j;
            break;
         }
      }
   }
   m_ndet++;
   m_det[m_ndet]=m_nhits;
   // printDebug();
}

void
FWTrackResidualDetailView::build (const FWModelId &id, const reco::Track* track, TEveWindowSlot* slot)
{
   prepareData(id, track);

   TCanvas *infoCanvas, *histCanvas;
   TEveWindow* wp = makePackCanvas(slot, infoCanvas, histCanvas);
   setEveWindow(wp);
   wp->SetElementName("Residual Detail View");
   
   // info
   infoCanvas->cd();
   makeLegend();
   histCanvas->cd();

   // draw histogram
   histCanvas->SetHighLightColor(-1);
   TH2F* h_res = new TH2F("h_resx","h_resx",10,-5.5,5.5,m_ndet,0,m_ndet);
   TPad* padX = new TPad("pad1","pad1", 0.2, 0., 0.8, 0.99);
   padX->SetBorderMode(0);
   padX->SetLeftMargin(0.2);
   padX->Draw();
   padX->cd();
   padX->SetFrameLineWidth(0);
   padX->Modified();
   h_res->SetDirectory(0);
   h_res->SetStats(kFALSE);
   h_res->SetTitle("");
   h_res->SetXTitle("residual");
   h_res->GetXaxis()->SetTickLength(0);
   h_res->GetYaxis()->SetTickLength(0);
   h_res->GetXaxis()->SetNdivisions(20);
   h_res->GetYaxis()->SetLabelSize(0.06);
   h_res->Draw();
   padX->SetGridy();

   float larray[9]={0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.5, 4.5, 5.5};
   float larray2[8];
   for(int l=0; l<8; l++) {
      float diff2=(larray[l+1]-larray[l])/2;
      larray2[l]=larray[l]+diff2;
      //  printf("(%.1f,%.1f),",larray[i],larray[i+1]);
   }

   int resi[2][64];
   for(int l=0; l<m_nhits; l++) {
      for(int k=0; k<8; k++) {
         if(fabs(res[0][l])==larray2[k])
            resi[0][l]=k;
         if(fabs(res[1][l])==larray2[k])
            resi[1][l]=k;
      }
   }

   TLine* lines[17];
   for(int l=0; l<17; l++) {
      int ix=l%9;
      int sign=1;
      sign = (l>8) ? -1 : 1;
      lines[l] = new TLine(sign*larray[ix],0,sign*larray[ix],m_ndet);
      if(l!=9)
         lines[l]->SetLineStyle(3);
      padX->cd();
      lines[l]->Draw();
   }

   float width=0.25;
   int filltype;
   Color_t color;
   Double_t box[4];
   padX->cd();

   for(int h=0; h<2; h++) {
      float height1=0;
      for(int j=0; j<m_ndet; j++) {
         // take only X res and Y pixel residals
         if (strcmp(m_det_tracker_str[substruct[m_det[j]]-1], "PXB") && h)
            continue;

         char det_str2[256];
         sprintf(det_str2,"%s/%i",m_det_tracker_str[substruct[m_det[j]]-1],subsubstruct[m_det[j]]);
         h_res->GetYaxis()->SetBinLabel(j+1, det_str2);

         int diff=m_det[j+1]-m_det[j];
         int k=0;
         width=1.0/diff;

         for(int l=m_det[j]; l<(m_det[j]+diff); l++) {
            //      g->SetPoint(l,resx[l],j+0.5);
            //	printf("%i, %f %f %f\n",l,resx[l],sign*larray[resxi[l]],sign*larray[resxi[l]+1]);
            int sign = (res[h][l]<0) ? -1 : 1;
            box[0] = (hittype[l]==0) ? sign*larray[resi[h][l]] : -5.5;
            box[2] = (hittype[l]==0) ? sign*larray[resi[h][l]+1] : 5.5;
            box[1] = height1+width*k;
            box[3] = height1+width*(k+1);

            if(stereo[l]==1) {
               color    = m_stereoCol;
               filltype = m_stereoFill;
            }
            else if(hittype[l]!=0) {
               color    = m_invalidCol;
               filltype = m_invalidFill;
            }
            else {
               filltype = h ? m_resYFill : m_resXFill;
               color    = h ? m_resYCol  : m_resXCol;
            }

            drawBox(box, color, filltype, h<1);
            k++;
         }
         height1 +=1;
      }
   }
   
   //  title
   const char* res_str= "residuals in Si detector local x-y coord.";
   TPaveText *pt = new TPaveText(0.0,0.91, 1,0.99,"blNDC");
   pt->SetBorderSize(0);
   pt->AddText(res_str);
   pt->Draw();
   
   histCanvas->cd();
   histCanvas->SetEditable(kFALSE);
}

double
FWTrackResidualDetailView::getSignedResidual (const DetIdToMatrix *detIdToGeo, unsigned int id, double resX)
{
   double local1[3] = { 0, 0, 0 };
   double local2[3] = { resX, 0, 0 };
   double global1[3], global2[3];
   const TGeoHMatrix *m = detIdToGeo->getMatrix(id);
   assert(m != 0);
   m->LocalToMaster(local1, global1);
   m->LocalToMaster(local2, global2);
   TVector3 g1 = global1;
   TVector3 g2 = global2;
   if (g2.DeltaPhi(g1) > 0)
      return resX;
   else return -resX;
}


void
FWTrackResidualDetailView::drawBox(Double_t *pos, Color_t fillCol, Int_t fillType, bool bg)
{
   // background
   if (bg)
   {
      TBox *b1 = new TBox(pos[0], pos[1], pos[2], pos[3]);
      b1->SetFillColor(fillCol);
      b1->Draw();
   }

   // fill style
   TBox *b2 = new TBox(pos[0], pos[1], pos[2], pos[3]);
   b2->SetFillStyle(fillType);
   b2->SetFillColor(kBlack);
   b2->Draw();

   //outline
   TBox *b3 = new TBox(pos[0], pos[1], pos[2], pos[3]);
   b3->SetFillStyle(0);
   b3->SetFillColor(1);
   b3->SetLineWidth(2);
   b3->Draw();
}

void
FWTrackResidualDetailView::printDebug()
{
   for(int i=0; i<m_ndet; i++)
   {
      std::cout <<"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
      std::cout << "idx " << i << " det[idx] " <<  m_det[i] << std::endl;
      std::cout << "m_det idx " << m_det[i] <<   std::endl;
      std::cout << "m_det_tracker_str idx " << substruct[m_det[i]]-1  << std::endl;
      printf("m_det[idx] %i m_det_tracker_str %s substruct %i\n",m_det[i], m_det_tracker_str[substruct[m_det[i]]-1], subsubstruct[m_det[i]]);
   }
}

void
FWTrackResidualDetailView::makeLegend()
{
   char mytext[256];
   Double_t fontsize = 0.07;
   TLatex* latex = new TLatex();
   latex->SetTextSize(fontsize);
   latex->Draw();
   Double_t x0 = 0.02;
   Double_t y = 0.95;

   // summary
   int nvalid=0;
   int npix=0;
   int nstrip=0;
   for(int i=0; i<m_nhits; i++)
   {
      if(hittype[i]==0) nvalid++;
      if(substruct[i]<3) npix++;
      else nstrip++;
   }

   latex->SetTextSize(fontsize);
   Double_t boxH = 0.25*fontsize;
   
   double yStep = 0.04;

   latex->DrawLatex(x0, y, "Residual:");
   y-= yStep;
   latex->DrawLatex(x0, y, "sgn(#hat{X}#bullet#hat{#phi}) #times #frac{X_{hit} - X_{traj}}{#sqrt{#sigma^{2}_{hit} + #sigma^{2}_{traj}}}" );
   y-= 2.5*yStep;
   sprintf(mytext,"layers hit: %i", m_ndet);
   latex->DrawLatex(x0, y, mytext);
   y -= yStep;
   sprintf(mytext,"valid Si hits: %i", nvalid);
   latex->DrawLatex(x0, y, mytext);
   y -= yStep;
   sprintf(mytext,"total Si hits: %i", m_nhits);
   latex->DrawLatex(x0, y, mytext);
   y -= yStep;
   sprintf(mytext,"valid Si pixel hits: %i", npix);
   latex->DrawLatex(x0, y, mytext);
   y -= yStep;
   sprintf(mytext,"valid Si strip hits: %i", nstrip);
   latex->DrawLatex(x0, y, mytext);
   

   Double_t pos[4];
   pos[0] = 0.4;
   pos[2] = 0.55;
   y -= yStep*2;
   sprintf(mytext,"X hit");
   latex->DrawLatex(x0, y, mytext);
   pos[1] = y; pos[3] = pos[1] + boxH;
   drawBox(pos, m_resXCol, m_resXFill);

   y -=  yStep;
   sprintf(mytext,"Y hit");
   latex->DrawLatex(x0, y, mytext);
   pos[1] = y; pos[3] = pos[1] + boxH;
   drawBox(pos, m_resYCol, m_resYFill, 0);

   y -= yStep;
   sprintf(mytext, "stereo hit");
   latex->DrawLatex(x0, y, mytext);
   pos[1] = y; pos[3] = pos[1] + boxH;
   drawBox(pos, m_stereoCol, m_stereoFill);

   y -= yStep;
   sprintf(mytext, "invalid hit");
   latex->DrawLatex(x0, y, mytext);
   pos[1] = y; pos[3] = pos[1] + boxH;
   drawBox(pos, m_invalidCol, m_invalidFill);
}

REGISTER_FWDETAILVIEW(FWTrackResidualDetailView, Residuals);
