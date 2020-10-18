/**
  GDALAllRegister,仅此一次
*/
void RegisterAll()
{
  if(GDALGetDriverByName("GTiff") == NULL) {
    GDALAllRegister();
  }
}

/**
所有subset转成一个tif
  @param fileName    输入文件名
  @param outfile     输出文件名
  @param pBandIndex  所需输出的数据集，用数组表示，从1开始，例如[1,3,5]，传入NULL表示全部数据集
  @param pBandCount  所需输出数据集个数，如果pBandIndex设置为NULL，此参数无作用
  @return
 */
void subsets2tif(const char *fileName, const char *outfile, \
                 int *pBandIndex, int pBandCount)
{
  int i, j;
  char *outFileName = NULL; //输出文件名

  if(outfile == NULL) {
    int charNum = strlen(fileName);
    charNum += 5;
    outFileName = new char[charNum];
    strcpy(outFileName, fileName);
    strcat(outFileName, ".tif") ;
  } else {
    int charNum = strlen(outfile);
    outFileName = new char[charNum + 1];
    strcpy(outFileName, outfile);
  }

  RegisterAll();//注册类型，打开影像必须加入此句
  GDALDataset *pDataSet = (GDALDataset *) GDALOpen(fileName, GA_ReadOnly);

  if(pDataSet == NULL) {
    printf("不能打开该文件，请检查文件是否存在！");
    return ;
  }

  //获取子数据集
  char **papszSUBDATASETS = pDataSet->GetMetadata("SUBDATASETS");
  int subdsNumber =  papszSUBDATASETS == NULL ? 1 : CSLCount(papszSUBDATASETS) / 2;
  char **vSubDataSets = new char*[subdsNumber]; //子数据集名称列表
  char **vSubDataDesc = new char*[subdsNumber]; //子数据集描述列表
  char *papszMetadata = NULL;

  //如果没有子数据集，则其本身就是一个数据集
  if(papszSUBDATASETS == NULL) {
    const char *Metadata = GDALGetDriverShortName((GDALDriverH)pDataSet);
    vSubDataSets[0] = new char[strlen(Metadata) + 1];
    vSubDataDesc[0] = new char[strlen(Metadata) + 1];
    strcpy(vSubDataSets[0], Metadata);
    strcpy(vSubDataDesc[0], Metadata);
  } else {
    int iCount = CSLCount(papszSUBDATASETS);  //计算子数据集的个数

    if(iCount <= 0) {   //没有子数据集,则返回
      GDALClose((GDALDriverH)pDataSet);
      return;
    }

    //将子数据集压入列表
    for(i = 0, j = 0; papszSUBDATASETS[i] != NULL; i++) {
      if(i % 2 != 0) {
        continue;
      }

      char *setInfo = papszSUBDATASETS[i];
      setInfo = strstr(setInfo, "=");

      if(setInfo[0] == '=') {
        memmove(setInfo, setInfo + 1, strlen(setInfo)); //提取元数据中子数据集名称
        vSubDataSets[j] = new char[strlen(setInfo) + 1];
        strcpy(vSubDataSets[j], setInfo);
      }

      char *descptn = papszSUBDATASETS[i + 1];
      descptn = strstr(descptn, "=");

      if(descptn[0] == '=') {
        memmove(descptn, descptn + 1, strlen(descptn)); //提取元数据中子数据集描述
        vSubDataDesc[j] = new char[strlen(descptn) + 1];
        strcpy(vSubDataDesc[j], descptn);
      }

      j++;
    }//end for
  }

  int Width, Height;
  GDALDriver *poDriver;
  const char *pszFormat = "GTiff";
  poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

  if(poDriver == NULL) {
    return;
  }

  //将每个子数据集转为对应的tif假设所有数据集大小相同
  bool init = false;
  int iBands = pBandIndex != NULL ? pBandCount : subdsNumber;
  int realBandCount = 0;

  for(i = 0; i < subdsNumber; i++) {
    if(pBandIndex != NULL) {
      bool runNext = true;

      for(j = 0; j < pBandCount; j++) {
        if(pBandIndex[j] == (i + 1)) {
          realBandCount = j + 1;
          runNext = false;
          break;
        }
      }

      if(runNext) {
        continue;
      }
    } else {
      realBandCount = i + 1;
    }

    char *dsName = vSubDataSets[i];
    GDALDataset *subDs = (GDALDataset *) GDALOpen(dsName, GA_ReadOnly);

    if(subDs == NULL) {
      continue;
    }

    Width = subDs->GetRasterXSize();
    Height = subDs->GetRasterYSize();
    void *subData;
    GDALRasterBand *poBand = subDs->GetRasterBand(1);
    GDALDataType eBufType = poBand->GetRasterDataType();

    switch(eBufType) {
      case GDT_Byte:
        subData = new char[Width * Height];
        break;

      case GDT_Int16:
        subData = new short[Width * Height];
        break;

      case GDT_Int32:
        subData = new int[Width * Height];
        break;

      case GDT_Float32:
        subData = new float[Width * Height];
        break;

      case GDT_Float64:
        subData = new double[Width * Height];
        break;

      default:
        subData = new double[Width * Height];
        break;
    }

    poBand->RasterIO(GF_Read, 0, 0, Width, Height, subData,\
                     Width, Height, eBufType, 0, 0);
    //写入影像
    GDALDataset *pDstDs = NULL;

    if(!init) { //创建影像
      char **papszOptions = NULL;
      //设置bsq或者 BIP bsq:BAND,bip:PIXEL
      papszOptions = CSLSetNameValue(papszOptions, "INTERLEAVE", "BAND");
      pDstDs = poDriver->Create(outFileName, Width, Height, \
                                iBands, eBufType, papszOptions);
      double geos[6];
      subDs->GetGeoTransform(geos);//变换参数
      char *pszProjection = NULL;
      char *pszPrettyWkt = NULL;
      //获取ds1坐标
      OGRSpatialReferenceH  hSRS;

      if(GDALGetProjectionRef(subDs) != NULL) {
        pszProjection = (char *) GDALGetProjectionRef(subDs);
        hSRS = OSRNewSpatialReference(NULL);

        if(OSRImportFromWkt(hSRS, &pszProjection) == CE_None) {
          OSRExportToPrettyWkt(hSRS, &pszPrettyWkt, FALSE);
          pDstDs->SetProjection(pszPrettyWkt);
        }
      }

      //设置坐标
      pDstDs->SetGeoTransform(geos);
      CPLFree(pszPrettyWkt);
      pDstDs->SetMetadata(subDs->GetMetadata());
      pDstDs->FlushCache();
      init = true;
    } else {
      //打开影像
      pDstDs = (GDALDataset *) GDALOpen(outFileName, GA_Update);
    }

    GDALRasterBand *poBandOut;
    poBandOut = pDstDs->GetRasterBand(realBandCount);
    poBandOut->SetScale(poBand->GetScale());
    poBandOut->SetOffset(poBand->GetOffset());
    poBandOut->SetUnitType(poBand->GetUnitType());
    poBandOut->SetColorInterpretation(poBand->GetColorInterpretation());
    poBandOut->SetDescription(poBand->GetDescription());
    poBandOut->SetNoDataValue(poBand->GetNoDataValue());
    //GDT_Float32和 **OutputImg 类型要对应！
    poBandOut->RasterIO(GF_Write, 0, 0, Width, Height, \
                        subData, Width, Height, eBufType, 0, 0);
    pDstDs->FlushCache();
    //关闭
    GDALClose(subDs);
    GDALClose(pDstDs);
    delete[] subData;
  }

  GDALClose(pDataSet);

  if(papszMetadata) {
    delete papszMetadata;
  }

  for(i = 0; i < subdsNumber; i++) {
    delete[] vSubDataDesc[i];
    delete[] vSubDataSets[i];
  }

  delete[] vSubDataDesc;
  vSubDataDesc = NULL;
  delete[] vSubDataSets;
  vSubDataSets = NULL;
  delete[] outFileName;
}