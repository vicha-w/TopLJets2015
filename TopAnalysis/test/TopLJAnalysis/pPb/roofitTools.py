import ROOT

"""
"""
def getEffSigma(var,pdf,wmin,wmax,step=0.1,epsilon=1e-4):

  cdf = pdf.createCdf(ROOT.RooArgSet(var))
  point=wmin
  points=[]
  mindiffTo05,median=999,-1
  maxPDF,mode=0,-1
  while (point <= wmax):
      var.setVal(point)
      pdfVal=pdf.getVal()
      if pdfVal>epsilon :
          cdfVal=cdf.getVal()
          points.append( (point,cdfVal) )
          if abs(cdfVal-0.5) < mindiffTo05 :
              mindiffTo05,median=abs(cdfVal-0.5),point
          if pdfVal>maxPDF:
              maxPDF,mode=pdfVal,point
      point+=step

  low,high=wmin,wmax
  width=high-low
  for i in xrange(0,len(points)):
      for j in xrange(i,len(points)):
          wy = points[j][1] - points[i][1]
          if abs(wy-0.683)<epsilon:
            wx = points[j][0] - points[i][0]
            if wx<width:
                low,high,width = points[i][0],points[j][0],wx
  return pdf.mean(var).getVal(),median,mode,high-low




"""
display the results of the fit
"""
def showFitResult(fitVar,data,pdf,categs,w,showComponents=[],rangeX=(0,400),outDir='plots/',paramList=[],tagTitle='',pdfAux=None,pfix=''):

    c=ROOT.TCanvas('c','c',500,500)
    c.SetTopMargin(0)
    c.SetLeftMargin(0)
    c.SetRightMargin(0)
    c.SetBottomMargin(0)
    p1 = ROOT.TPad('p1','p1',0.0,0.85,1.0,0.0)
    p1.SetRightMargin(0.05)
    p1.SetLeftMargin(0.12)
    p1.SetTopMargin(0.008)
    p1.SetBottomMargin(0.12)
    p1.Draw()
    c.cd()
    p2 = ROOT.TPad('p2','p2',0.0,0.85,1.0,1.0)
    p2.SetBottomMargin(0.005)
    p2.SetRightMargin(0.05)
    p2.SetLeftMargin(0.12)
    p2.SetTopMargin(0.05)
    p2.SetGridy(True)
    p2.Draw()

    #show fit result
    for tag in categs:
        p1.cd()
        p1.Clear()
        frame=w.var(fitVar).frame(ROOT.RooFit.Range(rangeX[0],rangeX[1]),ROOT.RooFit.Bins(50))
        if len(tag)!=0:
            redData = data.reduce(ROOT.RooFit.Cut("sample==sample::%s"%tag))
            w.cat('sample').setLabel(tag)
        else:
            redData=data
        redData.plotOn(frame)

        if pdfAux:
            pdfAux.plotOn(frame,
                          ROOT.RooFit.ProjWData(redData),
                          ROOT.RooFit.LineColor(ROOT.kGray),
                          ROOT.RooFit.DrawOption('l'),
                          ROOT.RooFit.LineWidth(1)
                          )
        if pdf:
            for icomp in xrange(0,len(showComponents)):
                comps=showComponents[icomp]
                if comps=='S_cor*' or comps=='S_*' or comps=='S_cor*,S_wro*':
                    color=ROOT.TColor.GetColor('#4575b4')
                    if comps=='S_cor*,S_wro*':color=ROOT.TColor.GetColor('#91bfdb')
                    if len(tag)!=0:
                        pdf.plotOn(frame,
                                   ROOT.RooFit.Components(comps),
                                   ROOT.RooFit.Slice(w.cat('sample'),tag),
                                   ROOT.RooFit.ProjWData(redData),
                                   ROOT.RooFit.LineColor(color),
                                   ROOT.RooFit.FillColor(color),
                                   ROOT.RooFit.FillStyle(1001),
                                   ROOT.RooFit.DrawOption('f'),
                                   ROOT.RooFit.LineWidth(1),
                                   ROOT.RooFit.MoveToBack())
                    else:
                        pdf.plotOn(frame,
                                   ROOT.RooFit.Components(comps),
                                   ROOT.RooFit.ProjWData(redData),
                                   ROOT.RooFit.LineColor(color),
                                   ROOT.RooFit.FillColor(color),
                                   ROOT.RooFit.FillStyle(1001),
                                   ROOT.RooFit.DrawOption('f'),
                                   ROOT.RooFit.LineWidth(1),
                                   ROOT.RooFit.MoveToBack())
                else:
                    if len(tag)!=0:
                        pdf.plotOn(frame,
                                   ROOT.RooFit.Components(comps),
                                   ROOT.RooFit.Slice(w.cat('sample'),tag),
                                   ROOT.RooFit.ProjWData(redData),
                                   ROOT.RooFit.LineStyle(2),
                                   ROOT.RooFit.LineColor(ROOT.kGray+icomp+1),
                                   ROOT.RooFit.LineWidth(1),
                                   ROOT.RooFit.MoveToBack())
                    else:
                        pdf.plotOn(frame,
                                   ROOT.RooFit.Components(comps),
                                   ROOT.RooFit.ProjWData(redData),
                                   ROOT.RooFit.LineStyle(2),
                                   ROOT.RooFit.LineColor(ROOT.kGray+icomp+1),
                                   ROOT.RooFit.LineWidth(1),
                                   ROOT.RooFit.MoveToBack())
            if len(tag)!=0:
                pdf.plotOn(frame,
                           ROOT.RooFit.Slice(w.cat('sample'),tag),
                           ROOT.RooFit.ProjWData(redData),
                           ROOT.RooFit.LineColor(ROOT.kBlue),
                           ROOT.RooFit.LineWidth(2))
            else:
                pdf.plotOn(frame,
                           ROOT.RooFit.ProjWData(redData),
                           ROOT.RooFit.LineColor(ROOT.kBlue),
                           ROOT.RooFit.LineWidth(2))

        frame.Draw()
        frame.GetYaxis().SetTitle("Entries")
        frame.GetYaxis().SetTitleOffset(1.0)
        frame.GetYaxis().SetTitleSize(0.05)
        frame.GetYaxis().SetLabelSize(0.04)
        frame.GetXaxis().SetTitleSize(0.05)
        frame.GetXaxis().SetLabelSize(0.04)
        frame.GetXaxis().SetTitle(w.var(fitVar).GetTitle())

        label = ROOT.TLatex()
        label.SetNDC()
        label.SetTextFont(42)
        label.SetTextSize(0.04)
        label.DrawLatex(0.6,0.92,'#bf{CMS} #it{preliminary}')
        if tagTitle!='' : label.DrawLatex(0.6,0.88,tagTitle)
        label.DrawLatex(0.6,0.84,'#chi^{2}=%3.2f'%frame.chiSquare())

        #print the PDF parameters on the canvas
        #paramList=[('Nsig','N_{cp}(t#bar{t})'),
        #                ('sig_mu_%s'%fitVar,'#mu'),
        #                ('sig_sigma_%s'%fitVar,'#sigma'),
        #               ('eb_%s'%j,'#epsilon_{b}')]
        if len(paramList)==0:
            iter = pdf.getParameters(redData).createIterator()
            iparam = iter.Next()
            while iparam :
                if not iparam.getAttribute('Constant'):
                    paramList.append( (iparam.GetName(),iparam.GetTitle() ) )
                iparam = iter.Next()

        ivar=0
        for var,tit in paramList:
            label.DrawLatex(0.6,
                            0.80-0.04*ivar,
                            '#scale[0.8]{%s=%3.2f#pm%3.2f}'%(tit,
                                                             w.var(var).getVal(),
                                                             w.var(var).getError()) )
            ivar+=1

        p2.cd()
        p2.Clear()
        if pdf:
            hpull = frame.pullHist()
            pullFrame=w.var(fitVar).frame(ROOT.RooFit.Range(rangeX[0],rangeX[1]))
            pullFrame.addPlotable(hpull,"P") ;
            pullFrame.Draw()
            pullFrame.GetYaxis().SetTitle("Pull")
            pullFrame.GetYaxis().SetTitleSize(0.25)
            pullFrame.GetYaxis().SetLabelSize(0.25)
            pullFrame.GetXaxis().SetTitleSize(0)
            pullFrame.GetXaxis().SetLabelSize(0)
            pullFrame.GetYaxis().SetTitleOffset(0.15)
            pullFrame.GetYaxis().SetNdivisions(4)
            pullFrame.GetYaxis().SetRangeUser(-3.1,3.1)
            pullFrame.GetXaxis().SetTitleOffset(0.8)

        c.cd()
        c.Modified()
        c.Update()
        #raw_input()
        for ext in ['png','pdf']:
            c.SaveAs('%s/%s_%s%s.%s'%(outDir,fitVar,tagTitle if tag=='' else tag,pfix,ext))

def showLikelihoods(pll,ll,var):

    c=ROOT.TCanvas('c','c',500,500)
    c.SetTopMargin(0.05)
    c.SetLeftMargin(0.12)
    c.SetRightMargin(0.03)
    c.SetBottomMargin(0.1)
    fitRes=(var.getVal(),var.getErrorHi(),var.getErrorLo())
    frame=var.frame( ROOT.RooFit.Bins(10), ROOT.RooFit.Range(fitRes[0]+2*fitRes[2],fitRes[0]+2*fitRes[1]) )
    pll.plotOn(frame, ROOT.RooFit.ShiftToZero(), ROOT.RooFit.ShowProgress() )
    ll.plotOn(frame, ROOT.RooFit.ShiftToZero(),ROOT.RooFit.LineStyle(2), ROOT.RooFit.ShowProgress() )
    frame.Draw()
    frame.GetYaxis().SetTitle('Likelihod')
    frame.GetXaxis().SetTitle( var.GetTitle() )
    label = ROOT.TLatex()
    label.SetNDC()
    label.SetTextFont(42)
    label.SetTextSize(0.04)
    label.DrawLatex(0.6,0.92,'#bf{CMS} #it{preliminary}')
    label.DrawLatex(0.6,0.88,'%s=%3.1f^{+%3.1f}_{-%3.1f}'%(var.GetTitle(),fitRes[0],fitRes[1],fitRes[2]))
    c.Modified()
    c.Update()
    #raw_input()