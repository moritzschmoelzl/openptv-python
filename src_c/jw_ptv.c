/****************************************************************************
 *****************************************************************************
 
 Author/Copyright:   Hans-Gerd Maas / Jochen Willneff
 
 Address:	      	Institute of Geodesy and Photogrammetry
 ETH - Hoenggerberg
 CH - 8093 Zurich
 
 Creation Date:	    took a longer time ...
 
 Description:	    target detection, correspondences and
 positioning with tclTk display
 -> 4 camera version
 
 Routines contained:    	only start_proc_c, init_proc_c, detection_proc_c left, pre_processing_c, pre_processing_c, Denis - 26/10/2010 
 
 ****************************************************************************/
#include "ptv.h"

#define nmax 20240

/*  global declarations for ptv  */
/*-------------------------------------------------------------------------*/

int	n_img;	       		      	/* no of images */
int	hp_flag=0;           	      	/* flag for highpass */
int	allCam_flag=0;           	      	/* flag for using all cams for points */
int	tiff_flag=0;           	      	/* flag for tiff header */
int pair_flag=0;					/*flag for accept pair */
int	chfield;       		       	/* flag for field mode */
int	nfix;	       	       	       	/* no. of control points */
int	num[4];	       		       	    /* no. of targets per image */
int numc[4];                        /* no. of targets in current image */
int nump[4];                        /* no. of targets in previous image */
int numn[4];                        /* no. of targets in next image */
int n_trac[4];	           	/* no. of tracks */
int	match=0;		      	/* no. of matches */
int	match2=0;	      	       	/* no. of matches in 2nd pass */

int	nr[4][4];		     	/* point numbers for man. ori */
int	imx, imy, imgsize;	      	/* image size */
int	zoom_x[4],zoom_y[4],zoom_f[4];  /* zoom parameters */
int	pp1=0, pp2=0, pp3=0, pp4=0,pp5=0;   	/* for man. orientation */
int	seq_first, seq_last;	       	/* 1. and last img of seq */
int	max_shake_points, max_shake_frames, step_shake;
int	demo_nr;		      	/* for demo purposes */
int	examine = 0;		       	/* for more detailed output */
int	dump_for_rdb;		       	/* # of dumpfiles for rdb */
int cr_sz;                          /* size of crosses */
int display;                        /* display flag */
int corp, corc, corn;
int m[4];
int trackallocflag = 0;      /* checkflag if mega, c4, t4 already allocated */
int mask;						/*checkmark for subtract mask*/

double	pix_x, pix_y;			      	/* pixel size */
double	ro;			      	        /* 200/pi */
double	cn, cnx, cny, csumg, eps0, corrmin;	/* correspondences par */
double 	rmsX, rmsY, rmsZ, mean_sigma0;		/* a priori rms */
double  X_lay[2], Zmin_lay[2], Zmax_lay[2];	/* illu. layer data */
double  db_scale;               /*dumbbell length, Beat Mai 2010*/  

FILE	*fp1, *fp2, *fp3, *fp4, *fpp;

char	img_name[4][256];      	/* original image names */
char   	img_lp_name[4][256]; 	/* lowpass image names */
char   	img_hp_name[4][256];   	/* highpass image names */
char   	img_cal[4][128];       	/* calibrayion image names */
char   	img_ori[4][128];       	/* image orientation data */
char   	img_ori0[4][128];      	/* orientation approx. values */
char   	img_addpar[4][128];    	/* image additional parameters */
char   	safety[4][128];
char    safety_addpar[4][128];
char   	img_addpar0[4][128];   	/* ap approx. values */
char   	seq_name[4][128];      	/* sequence names */
char	img_mask_name[4][256];	/* mask image names*/
char	img_mask_path[256];
char   	track_dir[128];	       	/* directory with dap track data */
char    fixp_name[128];
char   	res_name[128];	      	/* result destination */
char   	buf[256], val[256];	       	/* buffer */
char    name[128];  //Beat Dez 08
double  xp, yp; //Beat Dez 08



unsigned char	*img[4];      	/* image data */
unsigned char	*img_mask[4];	/* mask data */
unsigned char	*img_new[4];	/* image data for reducing mask */
unsigned char	*img0[4];      	/* image data for filtering etc */
unsigned char	*zoomimg;     	/* zoom image data */

Exterior       	Ex[4],sEx[4];	      	/* exterior orientation */
Interior       	I[4],sI[4];	       	/* interior orientation */
Glass       	G[4],sG[4];	       	/* glass orientation */
ap_52	       	ap[4],sap[4];	       	/* add. parameters k1,k2,k3,p1,p2,scx,she */
mm_np	       	mmp;	       	/* n-media parameters */
target	       	pix[4][nmax]; 	/* target pixel data */
target	       	pix0[4][12];    	/* pixel data for man_ori points */

target          *t4[4][4];
int             nt4[4][4];
int             x_calib[4][1000];
int             y_calib[4][1000];
int             z_calib[4][1000];
int 		ncal_points[4];

coord_2d       	crd[4][nmax];  	/* (distorted) metric coordinates */
coord_2d       	geo[4][nmax];  	/* corrected metric coordinates */
coord_3d       	fix[20096];     	/* testfield points coordinates */ //Beat changed it on 090325
n_tupel	       	con[nmax];     	/* list of correspondences */

corres	       	*c4[4];
trackparameters tpar;           /* tracking parameters */

mm_LUT	       	mmLUT[4];     	/* LUT for multimedia radial displacement */
coord_3d        *p_c3d;
P *mega[4];

/***************************************************************************/

int init_proc_c()
{
    int  i;
    //const char *valp;
    
    puts ("\n Multimedia Particle Positioning and Tracking Software \n");
    
    //valp = Tcl_GetVar(interp, "examine",  TCL_GLOBAL_ONLY);
    //examine = atoi (valp);
    
    ro = 200/M_PI;
    
    fpp = fopen ("parameters/pft_version.par", "r");
    if (fpp){
    }
    else{
        fpp = fopen ("parameters/pft_version.par", "w");
        fprintf(fpp,"%d\n", 0);
        fclose(fpp);
    }
    
    /*  read from main parameter file  */
    fpp = fopen_r ("parameters/ptv.par");
    
    fscanf (fpp, "%d\n", &n_img);
    
    for (i=0; i<4; i++)
    {
        fscanf (fpp, "%s\n", img_name[i]);
        fscanf (fpp, "%s\n", img_cal[i]);
    }
    fscanf (fpp, "%d\n", &hp_flag);
    fscanf (fpp, "%d\n", &allCam_flag);
    fscanf (fpp, "%d\n", &tiff_flag);
    fscanf (fpp, "%d\n", &imx);
    fscanf (fpp, "%d\n", &imy);
    fscanf (fpp, "%lf\n", &pix_x);
    fscanf (fpp, "%lf\n", &pix_y);
    fscanf (fpp, "%d\n", &chfield);
    fscanf (fpp, "%lf\n", &mmp.n1);
    fscanf (fpp, "%lf\n", &mmp.n2[0]);
    fscanf (fpp, "%lf\n", &mmp.n3);
    fscanf (fpp, "%lf\n", &mmp.d[0]);
    fclose (fpp);
    
    /* read illuminated layer data */
    fpp = fopen_r ("parameters/criteria.par");
    fscanf (fpp, "%lf\n", &X_lay[0]);
    fscanf (fpp, "%lf\n", &Zmin_lay[0]);
    fscanf (fpp, "%lf\n", &Zmax_lay[0]);
    fscanf (fpp, "%lf\n", &X_lay[1]);
    fscanf (fpp, "%lf\n", &Zmin_lay[1]);
    fscanf (fpp, "%lf\n", &Zmax_lay[1]);
    fscanf (fpp, "%lf", &cnx);
    fscanf (fpp, "%lf", &cny);
    fscanf (fpp, "%lf", &cn);
    fscanf (fpp, "%lf", &csumg);
    fscanf (fpp, "%lf", &corrmin);
    fscanf (fpp, "%lf", &eps0);
    fclose (fpp);
    
    mmp.nlay = 1;
    
    /* read sequence parameters (needed for some demos) */
    
    fpp = fopen_r ("parameters/sequence.par");
    
    for (i=0; i<4; i++)		fscanf (fpp, "%s\n", seq_name[i]);
    fscanf (fpp,"%d\n", &seq_first);
    fscanf (fpp,"%d\n", &seq_last);
    fclose (fpp);
    
    /* initialize zoom parameters and image positions */
    for (i=0; i<n_img; i++)
    {
        num[i] = 0;
        zoom_x[i] = imx/2; zoom_y[i] = imy/2; zoom_f[i] = 1;
    }
    imgsize = imx*imy;
    
    /* allocate memory for images */
    for (i=0; i<n_img; i++)
    {
        img[i] = (unsigned char *) calloc (imgsize, 1);
        if ( ! img[i])
        {
            printf ("calloc for img%d --> error\n", i);
            exit (1);
        }
    }
    
    for (i=0; i<n_img; i++)
    {
        img_mask[i] = (unsigned char *) calloc (imgsize, 1);
        if ( ! img_mask[i])
        {
            printf ("calloc for img_mask%d --> error\n", i);
            exit (1);
        }
    }
    
    for (i=0; i<n_img; i++)
    {
        img0[i] = (unsigned char *) calloc (imgsize, 1);
        if ( ! img0[i])
        {
            printf ("calloc for img0%d --> error\n", i);
            exit (1);
        }
    }
    
    for (i=0; i<n_img; i++)
    {
        img_new[i] = (unsigned char *) calloc (imgsize, 1);
        if ( ! img_new[i])
        {
            printf ("calloc for img_new%d --> error\n", i);
            exit (1);
        }
    }
    
    zoomimg = (unsigned char *) calloc (imgsize, 1);
    if ( ! zoomimg)
    {
        printf ("calloc for zoomimg --> error\n");
        // return TCL_ERROR; //denis 26-10-2010
        return 1;
    }
    
    //parameter_panel_init(interp);
    //cr_sz = atoi(Tcl_GetVar2(interp, "mp", "pcrossize",  TCL_GLOBAL_ONLY));
    
    display = 1;
    // return TCL_OK;
    
    // globals for correspondences, denis 01/11/2010
    match4_g=0;
    match3_g=0;
    match2_g=0;
    match1_g=0;
    //----------------------------------
    
    return 0;
    
}


int start_proc_c()
{
    int  i, k;
    unsigned char *im0 = img[0];
    
    /*  read from main parameter file  */
    printf("inside start_proc");
    fpp = fopen_r ("parameters/ptv.par");
    
    fscanf (fpp, "%d\n", &n_img);
    
    for (i=0; i<4; i++)
    {
        fscanf (fpp, "%s\n", img_name[i]);
        fscanf (fpp, "%s\n", img_cal[i]);
    }
    fscanf (fpp, "%d\n", &hp_flag);
    fscanf (fpp, "%d\n", &allCam_flag);  
    fscanf (fpp, "%d\n", &tiff_flag);
    fscanf (fpp, "%d\n", &imx);
    fscanf (fpp, "%d\n", &imy);
    fscanf (fpp, "%lf\n", &pix_x);
    fscanf (fpp, "%lf\n", &pix_y);
    fscanf (fpp, "%d\n", &chfield);
    fscanf (fpp, "%lf\n", &mmp.n1);
    fscanf (fpp, "%lf\n", &mmp.n2[0]);
    fscanf (fpp, "%lf\n", &mmp.n3);
    fscanf (fpp, "%lf\n", &mmp.d[0]);
    fclose (fpp);
    
    /* read illuminated layer data */
    fpp = fopen_r ("parameters/criteria.par");
    fscanf (fpp, "%lf\n", &X_lay[0]);
    fscanf (fpp, "%lf\n", &Zmin_lay[0]);
    fscanf (fpp, "%lf\n", &Zmax_lay[0]);
    fscanf (fpp, "%lf\n", &X_lay[1]);
    fscanf (fpp, "%lf\n", &Zmin_lay[1]);
    fscanf (fpp, "%lf\n", &Zmax_lay[1]);
    fscanf (fpp, "%lf", &cnx);
    fscanf (fpp, "%lf", &cny);
    fscanf (fpp, "%lf", &cn);
    fscanf (fpp, "%lf", &csumg);
    fscanf (fpp, "%lf", &corrmin);
    fscanf (fpp, "%lf", &eps0);
    fclose (fpp);
    
    mmp.nlay = 1;
    
    /* read sequence parameters (needed for some demos) */
    
    fpp = fopen_r ("parameters/sequence.par");
    
    for (i=0; i<4; i++)		fscanf (fpp, "%s\n", seq_name[i]);
    fscanf (fpp,"%d\n", &seq_first);
    fscanf (fpp,"%d\n", &seq_last);
    fclose (fpp);
    
    /*  create file names  */
    for (i=0; i<n_img; i++)
    {
        strcpy (img_lp_name[i], img_name[i]); strcat (img_lp_name[i], "_lp");
        strcpy (img_hp_name[i], img_name[i]); strcat (img_hp_name[i], "_hp");
        strcpy (img_ori[i], img_cal[i]);  strcat (img_ori[i], ".ori");
        strcpy (img_addpar[i], img_cal[i]); strcat (img_addpar[i],".addpar");
    }
    
    
    
    /*  read orientation and additional parameters  */
    for (i=0; i<n_img; i++)
    {
        read_ori (&Ex[i], &I[i], &G[i], img_ori[i], &(ap[i]), img_addpar[i],
            NULL);
        rotation_matrix (Ex[i], Ex[i].dm); // Why, if it's read from a file?
    }
    
    /* read and display original images */
	for (i=0; i<n_img; i++)
    {
        /* reading */
        sprintf(val, "camcanvas %d", i+1);
        //  Tcl_Eval(interp, val);
        
        
        
        // read_image (interp, img_name[i], img[i]);
        // sprintf(val, "newimage %d", i+1);
        
        //  Tcl_Eval(interp, val);
        //  sprintf(val, "keepori %d", i+1);
        // Tcl_Eval(interp, val);
    }
    
    printf("image0:\n");
    for (i=0;i<50;i++) {
        printf("val1 =%d",im0[i]);
    }
    
    /* Make sure arrays of tracking-related data are allocated */
    if (!trackallocflag) {
        allocate_tracking_structs(t4, c4, mega, n_img, M);
    }
    
    //return TCL_OK; denis 26-10-2010
	return 0;
}

int pre_processing_c ()
{
    int i_img, sup, i;
    
    //Tk_PhotoHandle img_handle;
    //Tk_PhotoImageBlock img_block;
    
    sprintf(val, "Filtering with Highpass");
    // Tcl_SetVar(interp, "tbuf", val, TCL_GLOBAL_ONLY);
    // Tcl_Eval(interp, ".text delete 2");
    //Tcl_Eval(interp, ".text insert 2 $tbuf");
    
    /* read support of unsharp mask */
    fpp = fopen ("parameters/unsharp_mask.par", "r");
    if ( fpp == 0) { sup = 12;}
    else	{ fscanf (fpp, "%d\n", &sup); fclose (fpp); }
    
    //_____________________Matthias subtract mask__________________________
    
    
    /* Matthias JULI 08 read checkmark for masks and create mask names*/
    
    fpp = fopen_r ("parameters/targ_rec.par");
    for (i=0; i<14; i++){
        fscanf (fpp, "%d", &mask);      /*checkmark for subtract mask */
    }
    fscanf (fpp, "%s\n", img_mask_path);
    fclose (fpp);
    /*read mask names*/
    strcpy (img_mask_name[0], img_mask_path); strcat (img_mask_name[0], ".0");
    strcpy (img_mask_name[1], img_mask_path); strcat (img_mask_name[1], ".1");
    strcpy (img_mask_name[2], img_mask_path); strcat (img_mask_name[2], ".2");
    strcpy (img_mask_name[3], img_mask_path); strcat (img_mask_name[3], ".3");
    
    /*if the checkmark is set, read mask-image and subtract it from the filtered-original image.*/
    if (mask==1)
    {//read mask image
        for (i_img=0; i_img<n_img; i_img++)
        {
            //read_image (interp, img_mask_name[i_img], img_mask[i_img]);//read mask
            //highpass (img_name[i_img], img[i_img], img[i_img], sup, 0, chfield, i_img);//highpass original image
            highpass (img[i_img], img[i_img], sup, 0, chfield);
            subtract_mask (img[i_img], img_mask[i_img], img_new[i_img]); //subtract mask from original image
            copy_images (img_new[i_img], img[i_img]);//copy subtracted imgage on the original image
            
            //if (display) {
            // img_handle = Tk_FindPhoto( interp, "temp");
            // Tk_PhotoGetImage (img_handle, &img_block);
            // tclimg2cimg (interp, img[i_img], &img_block);
            
            sprintf(val, "newimage %d", i_img+1);
            //  Tcl_GlobalEval(interp, val);
            // }
        }
    }//end if
    
    if (mask==2)//Beat April 090402 was ==0
    {
        for (i_img=0; i_img<n_img; i_img++)
        {
            //highpass (img_name[i_img], img[i_img], img[i_img], sup, 0, chfield, i_img);//highpass original image
            highpass (img[i_img], img[i_img], sup, 0, chfield);//highpass original image
            
            /*      if (display) {*/
            /*      img_handle = Tk_FindPhoto( interp, "temp");*/
            /*      Tk_PhotoGetImage (img_handle, &img_block);*/
            /*      tclimg2cimg (interp, img[i_img], &img_block);*/
            
            /*      sprintf(val, "newimage %d", i_img+1);*/
            /*      Tcl_GlobalEval(interp, val);*/
            /*      }*/
        }
    }//end if
    
    /*------------------------------------------------------------*/
    
    
    sprintf(val, "...done");
    /*  Tcl_SetVar(interp, "tbuf", val, TCL_GLOBAL_ONLY);*/
    /*  Tcl_Eval(interp, ".text delete 3");*/
    /*  Tcl_Eval(interp, ".text insert 3 $tbuf");*/
    
    // return TCL_OK;
    return 0;
    
}


int detection_proc_c() 
{
    int	       	i, i_img, j;
    int	       	xmin, pft_version=3;
    char val[256];
    char filename[256];
    FILE	*FILEIN;
    
    /* process info */
    sprintf(val, "Detection of Particles");
    strcpy(val, "");
    
    /* xmin set to 10 so v_line is not included in detection, in future xmin should
     be set to 0, peakfitting has to be changed too */
    xmin=0;
    
    /*  read pft version  */
    fpp = fopen ("parameters/pft_version.par", "r");
    if (fpp){
        fscanf (fpp, "%d\n", &pft_version);
        pft_version=pft_version+3;
        fclose (fpp);
    }
    else{
        fpp = fopen ("parameters/pft_version.par", "w");
        fprintf(fpp,"%d\n", 0);
        fclose(fpp);
    }
    
    
    /* reset zoom values */
    for (i_img=0; i_img<n_img; i_img++)
    {
        zoom_x[i_img] = imx/2; zoom_y[i_img] = imy/2;  zoom_f[i_img] = 1;
    }
    
    /*copy images because the target recognition will set greyvalues to 0*/
    for (i_img=0; i_img<n_img; i_img++)
    {
        copy_images (img[i_img], img0[i_img]);
    }
    
    /* target recognition */
    for (i_img=0; i_img<n_img; i_img++)
    {
        switch (pft_version)
        {
            case 3:	/* pft with profile and distance check */
                /* newest version */
                xmin=0; /* vertical line restriction */
                num[i_img] = peak_fit_new (img[i_img],
                                           "parameters/targ_rec.par",
                                           xmin, imx, 1, imy, pix[i_img], i_img);
                break;
                
            case 0:	/* without peak fitting technique */
                simple_connectivity (img[i_img], img0[i_img],
                                     "parameters/targ_rec.par",
                                     xmin, imx, 1, imy, pix[i_img], i_img, &num[i_img]);
                break;
                
            case 1:	/* with old (but fast) peak fitting technique */
                targ_rec (img[i_img], img0[i_img],
                          "parameters/targ_rec.par",
                          xmin, imx, 1, imy, pix[i_img], i_img, &num[i_img]);
                break;
                
            case 4: /* new option for external image processing routines */
                /* added by Alex, 19.04.10 */
                /* this works here only for the pre-processing stage, see img_name[i_img] is not from a sequence */
                
                nt4[3][i_img] = read_targets(pix[i_img], img_name[i_img], 0);
                num[i_img] = nt4[3][i_img];
                
                printf("pix.x0=%d\n",pix[i_img][0].x);
                printf("pix.y0=%d\n",pix[i_img][0].y);
                
                break;
        }
        
        printf("pix.x0=%f\n",pix[0][0].x);
        printf("pix.y0=%f\n",pix[0][0].y);
        sprintf (buf,"%d: %d,  ", i_img+1, num[i_img]);
        strcat(val, buf);
        
        /* proper sort of targets in y-direction for later binary search */
        /* and for dimitris' tracking */
        quicksort_target_y (pix[i_img], num[i_img]);
        
        /* reorganize target numbers */
        for (i=0; i<num[i_img]; i++)  pix[i_img][i].pnr = i;
    }
    printf("pix.x01=%f\n",pix[0][0].x);
	printf("pix.y01=%f\n",pix[0][0].y);
    
    sprintf (buf, "Number of detected particles per image");
    /*  Tcl_SetVar(interp, "tbuf", buf, TCL_GLOBAL_ONLY);*/
    /*  Tcl_Eval(interp, ".text delete 2");*/
    /*  Tcl_Eval(interp, ".text insert 2 $tbuf");*/
    
    /*  Tcl_SetVar(interp, "tbuf", val, TCL_GLOBAL_ONLY);*/
    /*  Tcl_Eval(interp, ".text delete 3");*/
    /*  Tcl_Eval(interp, ".text insert 3 $tbuf");*/
    
    printf("%s\n", val);
    return 1;
}

//int correspondences_proc_c (ClientData clientData, Tcl_Interp* interp, int argc, const char** argv)
int correspondences_proc_c ()
{
    int	i, i_img, frame;
    double x,y;
    char filename[256];
    
    puts ("\nTransformation to metric coordinates\n");
    
    /* rearrange point numbers after manual deletion of points */
    for (i_img=0; i_img<n_img; i_img++)
        for (i=0; i<num[i_img]; i++)  pix[i_img][i].pnr = i;
    /* transformations pixel coordinates -> metric coordinates */
    /* transformations metric coordinates -> corrected metric coordinates */
    for (i_img=0; i_img<n_img; i_img++)
    {
        for (i=0; i<num[i_img]; i++)
        {
            pixel_to_metric (pix[i_img][i].x, pix[i_img][i].y,
                             imx,imy, pix_x, pix_y,
                             &crd[i_img][i].x, &crd[i_img][i].y, chfield);
            crd[i_img][i].pnr = pix[i_img][i].pnr;
            
            x = crd[i_img][i].x - I[i_img].xh;
            y = crd[i_img][i].y - I[i_img].yh;
            correct_brown_affin (x, y, ap[i_img], &geo[i_img][i].x, &geo[i_img][i].y);
            
            geo[i_img][i].pnr = crd[i_img][i].pnr;
        }
    }
    
    /* sort coordinates for binary search in correspondences_proc */
    for (i_img=0; i_img<n_img; i_img++)
    {
        quicksort_coord2d_x (geo[i_img], num[i_img]);
    }
    
    
    /* init multimedia radial displacement LUTs */
    /* ======================================== */
    
    if ( !mmp.lut && (mmp.n1 != 1 || mmp.n2[0] != 1 || mmp.n3 != 1))
    {
        puts ("Init multimedia displacement LUTs");
        for (i_img=0; i_img<n_img; i_img++) init_mmLUT(i_img);
        mmp.lut = 1;
    }
    
    //correspondences_4 ( argv);
    correspondences_4 ();
    
    /* --------------- */
    /* save pixel coords for tracking */
    for (i_img=0; i_img<n_img; i_img++)
    {
        /* This is a workaround for the globals-laden handling of file names.
        it should be removed when we get to removing the globals here. */
        for (i = strlen(img_name[i_img]) - 1; i >= 0; i--) {
            if ((img_name[i_img][i] > 0x39) || (img_name[i_img][i] < 0x30))
                break;
        }
        i++;
        strncpy(filename, img_name[i_img], i);
        filename[i] = '\0';
        
        sscanf(img_name[i_img] + i, "%d", &frame);
        printf("file %s, frame %d", filename, frame);
        write_targets(pix[i_img], num[i_img], filename, frame);
    }
    
    // return TCL_OK;
    return 0;
}


int calibration_proc_c (int sel)
{
    int i, j,  i_img, k, n, sup,dummy,multi,planes;
    int prev, next; 
    double dummy_float;
    int intx1, inty1, intx2, inty2;
    coord_2d    	apfig1[11][11];	/* regular grid for ap figures */
    coord_2d     	apfig2[11][11];	/* ap figures */
    coord_3d     	fix4[4];       	/* object points for preorientation */
    coord_2d     	crd0[4][12];    	/* image points for preorientation */
    char	       	multi_filename[10][256],filename[256], val[256];
    const char *valp;
    
    FILE	*FILEIN;
    char	filein[256];
    FILE	*FILEIN_ptv;
    char	filein_ptv[256];
    FILE	*FILEIN_T;
    char	filein_T[256];
    int filenumber;
    int dumy,frameCount,currentFrame;
    int a[4],a1[4],a2[4],success=1;
    double residual;
    
    // Tk_PhotoHandle img_handle;
    // Tk_PhotoImageBlock img_block;
    
    /* read support of unsharp mask */
    fp1 = fopen ("parameters/unsharp_mask.par", "r");
    if (! fp1)	sup = 12;
    else	{ fscanf (fp1, "%d\n", &sup); fclose (fp1); }
    
    /* Get Selection value from TclTk */
    
    /*  valp = Tcl_GetVar(interp, "sel",  TCL_GLOBAL_ONLY);*/
    /*  sel = atoi (valp);*/
    
    printf("\n Selection = %d Alex \n", sel);
    
    /* Beat Mai 2007 to set the variable examine for mulit-plane calibration*/
    fp1 = fopen_r ("parameters/examine.par");
    fscanf (fp1,"%d\n", &dummy);
    fscanf (fp1,"%d\n", &multi);
    fclose (fp1);
    if (dummy==1){
        examine=4;
    }
    else{
        examine=0;
    }
    printf("after 1\n");
    /*Oswald Juni 2008 accept pairs-------------------------------*/
    
    
    fp1 = fopen_r ("parameters/cal_ori.par");
    fscanf (fp1,"%s\n", fixp_name);
    printf("after 2\n");
    for (i=0; i<4; i++)
	{
        fscanf (fp1, "%s\n", img_name[i]);
        fscanf (fp1, "%s\n", img_ori0[i]);
	}
    printf("after 2.5\n");
    fscanf (fp1, "%d\n", &tiff_flag);
    printf("after 2.6\n");
    fscanf (fp1, "%d\n", &pair_flag);
    fclose (fp1);
    printf("after 3\n");
    if (pair_flag==1){
        int OSWALDDUMMY=1;
    }
    else{
        int OSWALDDUMMY=0;
    }
    
    printf("after 4\n");
    ///////////////////////////////////////////////////////////////////////////////
    
    switch (sel)
    {
        case 1: /*  read calibration parameter file  */
            fp1 = fopen_r ("parameters/cal_ori.par");
            fscanf (fp1,"%s\n", fixp_name);
            printf(" Checkpoint1 ");
            printf(" n_img= %d\n",n_img);
            
            for (i=0; i<4; i++)
            {
                fscanf (fp1, "%s\n", img_name[i]);
                fscanf (fp1, "%s\n", img_ori0[i]);
            }
            fscanf (fp1, "%d\n", &tiff_flag);
            fscanf (fp1, "%d\n", &pair_flag);
            fscanf (fp1, "%d\n", &chfield);
            fclose (fp1);
            
            /*  create file names  */
            printf("Checkpoint2 \n");
            printf("\n n_img=%d\n",n_img);
            for (i=0; i<n_img; i++)
            {
                strcpy (img_ori[i], img_name[i]);
                strcat (img_ori[i], ".ori");
                strcpy (img_addpar0[i], img_name[i]);
                strcat (img_addpar0[i], ".addpar0");
                strcpy (img_addpar[i], img_name[i]);
                strcat (img_addpar[i], ".addpar");
                strcpy (img_hp_name[i], img_name[i]);
                strcat (img_hp_name[i], "_hp");
            }
            strcpy (safety[0], "safety_0");
            strcat (safety[0], ".ori");
            strcpy (safety[1], "safety_1");
            strcat (safety[1], ".ori");
            strcpy (safety[2], "safety_2");
            strcat (safety[2], ".ori");
            strcpy (safety[3], "safety_3");
            strcat (safety[3], ".ori");
            strcpy (safety_addpar[0], "safety_0");
            strcat (safety_addpar[0], ".addpar");
            strcpy (safety_addpar[1], "safety_1");
            strcat (safety_addpar[1], ".addpar");
            strcpy (safety_addpar[2], "safety_2");
            strcat (safety_addpar[2], ".addpar");
            strcpy (safety_addpar[3], "safety_3");
            strcat (safety_addpar[3], ".addpar");
            
            //printf("\n Checkpoint4 \n");
            for (i=0; i<50; i++)
            {
                printf("img0=%d\n",img[0][i]);
            }
            // we take care of this in python, Denis //07.11.2010
            /*      for (i=0; i<n_img; i++)*/
            /*	{*/
            
            /*	  zoom_x[i] = imx/2, zoom_y[i] = imy/2, zoom_f[i] = 1;*/
            
            /*	  read_image (interp, img_name[i], img[i]);*/
            
            /*	  sprintf(val, "camcanvas %d", i+1);*/
            /*	  Tcl_Eval(interp, val);*/
            
            /*	  img_handle = Tk_FindPhoto( interp, "temp");*/
            /*	  Tk_PhotoGetImage (img_handle, &img_block);*/
            /*	  tclimg2cimg (interp, img[i], &img_block);*/
            
            /*	  sprintf(val, "newimage %d", i+1);*/
            /*	  Tcl_Eval(interp, val);*/
            /*	}*/
            
            break;
            
            
        case 2: puts ("Detection procedure"); strcpy(val,"");
            
            /* Highpass Filtering */
            pre_processing_c ();
            
            /* reset zoom values */
            for (i=0; i<n_img; i++)
            {
                zoom_x[i] = imx/2; zoom_y[i] = imy/2; zoom_f[i] = 1;
            }
            
            /* copy images because the target recognition
             will set greyvalues to zero */
            printf("\n after high pass inside detection");
            for (i=0; i<n_img; i++)
            {
                copy_images (img[i], img0[i]);
            }
            
            /* target recognition */
            for (i=0; i<n_img; i++)
            {
                targ_rec (img[i], img0[i], "parameters/detect_plate.par",
                          0, imx, 1, imy, pix[i], i, &num[i]);
                
                
                sprintf (buf,"image %d: %d,  ", i+1, num[i]);
                strcat(val, buf);
                
                if (num[i] > nmax)  exit (1);
            }
            printf("\n after targ_rec inside detection");
            /* save pixel coord as approx. for template matching */
            if (examine)	for (i=0; i<n_img; i++)
            {
                sprintf (filename, "%s_pix", img_name[i]);
                fp1 = fopen (filename, "w");
                for (j=0; j<num[i]; j++)
                    fprintf (fp1, "%4d  %8.3f  %8.3f\n",
                             pix[i][j].pnr, pix[i][j].x, pix[i][j].y);
                
                fclose (fp1);
            }
            
            sprintf(buf,"Number of detected targets, interaction enabled");
            /*      Tcl_SetVar(interp, "tbuf", buf, TCL_GLOBAL_ONLY);*/
            /*      Tcl_Eval(interp, ".text delete 2");*/
            /*      Tcl_Eval(interp, ".text insert 2 $tbuf");*/
            /*      Tcl_SetVar(interp, "tbuf", val, TCL_GLOBAL_ONLY);*/
            /*      Tcl_Eval(interp, ".text delete 3");*/
            /*      Tcl_Eval(interp, ".text insert 3 $tbuf");*/
            break;
            
        case 4: /* read pixel coordinates of older pre-orientation */
            
            /* read point numbers of pre-clicked points */
            fp1 = fopen_r ("parameters/man_ori.par");
            for (i=0; i<n_img; i++)
            {
                fscanf (fp1, "%d %d %d %d \n",
                        &nr[i][0], &nr[i][1], &nr[i][2], &nr[i][3]);
            }
            fclose (fp1);
            
            /* read coordinates of pre-clicked points */
            fp1 = fopen ("man_ori.dat", "r");
            if (! fp1)	break;
            for (i_img=0; i_img<n_img; i_img++)	for (i=0; i<4; i++)
            {
                fscanf (fp1, "%lf %lf\n",
                        &pix0[i_img][i].x, &pix0[i_img][i].y);
                /*	  drawcross (interp,  (int) pix0[i_img][i].x,*
                 /*		     (int) pix0[i_img][i].y, cr_sz+2, i_img, "red");*/
                /*	  draw_pnr (interp, (int) pix0[i_img][i].x, (int) pix0[i_img][i].y,*/
                /*		    nr[i_img][i], i_img, "red");*/
                
            }
            fclose (fp1);
            
            break;
        case 9: puts ("Plot initial guess");
            for (i=0; i<n_img; i++)
            {
                /* read control point coordinates for man_ori points */
                fp1 = fopen_r (fixp_name);
                k = 0;
                while ( fscanf (fp1, "%d %lf %lf %lf", &fix[k].pnr,
                                &fix[k].x, &fix[k].y, &fix[k].z) != EOF) k++;
                fclose (fp1);
                nfix = k;
                
                /* take clicked points from control point data set */
                for (j=0; j<4; j++)	for (k=0; k<nfix; k++)
                {
                    if (fix[k].pnr == nr[i][j])	fix4[j] = fix[k];
                }
                
                /* get approx for orientation and ap */
                read_ori (&Ex[i], &I[i], &G[i], img_ori0[i], &(ap[i]), 
                    img_addpar0[i], "addpar.raw");
                
                /* transform clicked points */
                for (j=0; j<4; j++)
                {
                    pixel_to_metric (pix0[i][j].x, pix0[i][j].y,
                                     imx,imy, pix_x, pix_y,
                                     &crd0[i][j].x, &crd0[i][j].y,
                                     chfield);
                    correct_brown_affin (crd0[i][j].x, crd0[i][j].y, ap[i],
                                         &crd0[i][j].x, &crd0[i][j].y);
                }
                
                /* raw orientation with 4 points */
                raw_orient_v3 (Ex[i], I[i], G[i], ap[i], mmp, 4, fix4, crd0[i], &Ex[i],&G[i],1);
                
                /* sorting of detected points by back-projection */
                just_plot (Ex[i], I[i], G[i], ap[i], mmp,
                           imx,imy, pix_x,pix_y,
                           nfix, fix,  chfield, i);
                
                /*write artifical images*/
                
                
            }
            
            break;
        case 5: puts ("Sort grid points - Alex test 18.5.11");
            for (i=0; i<n_img; i++)
            {
                /* read control point coordinates for man_ori points */
                fp1 = fopen_r (fixp_name);
                k = 0;
                while ( fscanf (fp1, "%d %lf %lf %lf", &fix[k].pnr,
                                &fix[k].x, &fix[k].y, &fix[k].z) != EOF) k++;
                fclose (fp1);
                nfix = k;
                
                /* take clicked points from control point data set */
                for (j=0; j<4; j++)	for (k=0; k<nfix; k++)
                {
                    if (fix[k].pnr == nr[i][j])	fix4[j] = fix[k];
                }
                
                /* get approx for orientation and ap */
                read_ori (&Ex[i], &I[i], &G[i], img_ori0[i], &(ap[i]),
                    img_addpar0[i], "addpar.raw");
                
                /* transform clicked points */
                for (j=0; j<4; j++)
                {
                    pixel_to_metric (pix0[i][j].x, pix0[i][j].y,
                                     imx,imy, pix_x, pix_y,
                                     &crd0[i][j].x, &crd0[i][j].y,
                                     chfield);
                    correct_brown_affin (crd0[i][j].x, crd0[i][j].y, ap[i],
                                         &crd0[i][j].x, &crd0[i][j].y);
                }
                
                /* raw orientation with 4 points */
                raw_orient_v3 (Ex[i], I[i], G[i], ap[i], mmp, 4, fix4, crd0[i], &Ex[i],&G[i],0); //Beat Nov 2008
                sprintf (filename, "raw%d.ori", i);
                write_ori (Ex[i], I[i], G[i], filename);
                
                /* sorting of detected points by back-projection */
                sortgrid_man (Ex[i], I[i], G[i], ap[i], mmp,
                              imx,imy, pix_x,pix_y,
                              nfix, fix, num[i], pix[i], chfield, i);
                
                /* adapt # of detected points */
                num[i] = nfix;
                
                
                
                ncal_points[i]=nfix;
                for (j=0; j<nfix; j++)
                {
                    //if (pix[i][j].pnr < 0)	continue;
// Alex, 18.05.11                    
//                     if (pix[i][j].pnr < 0)
//                     {
//                         z_calib[i][j]=pix[i][j].pnr; 
//                         printf("z_calib[%d][%d]<0: %d\n",i,j,z_calib[i][j]);   
//                     }
//                     else
//                     {
//                         z_calib[i][j]=fix[j].pnr; 
//                         printf("z_calib[%d][%d]>0: %d\n",i,j,z_calib[i][j]);
//                     }
                    
                    
                    
                    intx1 = (int) pix[i][j].x ;
                    inty1 = (int) pix[i][j].y ;
                    
                    //   x_calib[i][j]=intx1;
                    //   y_calib[i][j]=inty1;
                    
                    /*	      drawcross (interp, intx1, inty1, cr_sz, i, "white");*/
                    /*	      draw_pnr (interp, intx1, inty1, fix[j].pnr, i, "white");*/
                }
            }
            
            /* dump dataset for rdb */
            if (examine == 4)
            {
                /* create filename for dumped dataset */
                sprintf (filename, "dump_for_rdb");
                fp1 = fopen (filename, "w");
                
                /* write # of points to file */
                fprintf (fp1, "%d\n", nfix);
                
                /* write point and image coord to file */
                for (i=0; i<nfix; i++)
                {
                    fprintf (fp1, "%4d %10.3f %10.3f %10.3f   %d    ",
                             fix[i].pnr, fix[i].x, fix[i].y, fix[i].z, 0);
                    for (i_img=0; i_img<n_img; i_img++)
                    {
                        if (pix[i_img][i].pnr >= 0)
                        {
                            /* transform pixel coord to metric */
                            pixel_to_metric (pix[i_img][i].x,
                                             pix[i_img][i].y, imx,imy, pix_x, pix_y,
                                             &crd[i_img][i].x, &crd[i_img][i].y,
                                             chfield);
                            fprintf (fp1, "%4d %8.5f %8.5f    ",
                                     pix[i_img][i].pnr,
                                     crd[i_img][i].x, crd[i_img][i].y);
                        }
                        else
                        {
                            fprintf (fp1, "%4d %8.5f %8.5f    ",
                                     pix[i_img][i].pnr, 0.0, 0.0);
                        }
                    }
                    fprintf (fp1, "\n");
                }
                fclose (fp1);
                printf ("dataset dumped into %s\n", filename);
            }
            break;
            
        case 14: puts ("Sortgrid = initial guess");
            
            for (i=0; i<n_img; i++)
            {
                /* read control point coordinates for man_ori points */
                fp1 = fopen_r (fixp_name);
                k = 0;
                while ( fscanf (fp1, "%d %lf %lf %lf", &fix[k].pnr,
                                &fix[k].x, &fix[k].y, &fix[k].z) != EOF) k++;
                fclose (fp1);
                nfix = k;
                
                /* take clicked points from control point data set */
                for (j=0; j<4; j++)	for (k=0; k<nfix; k++)
                {
                    if (fix[k].pnr == nr[i][j])	fix4[j] = fix[k];
                }
                
                /* get approx for orientation and ap */
                read_ori (&Ex[i], &I[i], &G[i], img_ori0[i], &(ap[i]),
                    img_addpar0[i], "addpar.raw");
                
				/* transform clicked points */
				for (j=0; j<4; j++)
				{
					pixel_to_metric (pix0[i][j].x, pix0[i][j].y,
                                     imx,imy, pix_x, pix_y,
                                     &crd0[i][j].x, &crd0[i][j].y,
                                     chfield);
					correct_brown_affin (crd0[i][j].x, crd0[i][j].y, ap[i],
                                         &crd0[i][j].x, &crd0[i][j].y);
				}
                
				/* raw orientation with 4 points */
				raw_orient_v3 (Ex[i], I[i], G[i], ap[i], mmp, 4, fix4, crd0[i], &Ex[i],&G[i],1); /* correction 0 to 1 , al*/
				sprintf (filename, "raw%d.ori", i);
				write_ori (Ex[i], I[i], G[i], filename);
                
				/* sorting of detected points by back-projection */
				sortgrid_man (Ex[i], I[i], G[i], ap[i], mmp,
                              imx,imy, pix_x,pix_y,
                              nfix, fix, num[i], pix[i], chfield, i);
                
				/* adapt # of detected points */
				num[i] = nfix;
                
				for (j=0; j<nfix; j++)
				{
					if (pix[i][j].pnr < 0) z_calib[i][j]=pix[i][j].pnr;
				    else  z_calib[i][j]=fix[j].pnr;
                    intx1 = (int) pix[i][j].x ;
                    inty1 = (int) pix[i][j].y ;
                    /*					if (pix[i][j].pnr < 0)	continue;*/
                    /*					intx1 = (int) pix[i][j].x ;*/
                    /*					inty1 = (int) pix[i][j].y ;*/
                    
                    /*					drawcross (interp, intx1, inty1, cr_sz, i, "yellow");*/
                    /*					draw_pnr (interp, intx1, inty1, fix[j].pnr, i, "yellow");*/
				}
            }
            
            break;  
            
            
            
        case 6: puts ("Orientation"); strcpy(buf, "");
            
            strcpy (safety[0], "safety_0");
            strcat (safety[0], ".ori");
            strcpy (safety[1], "safety_1");
            strcat (safety[1], ".ori");
            strcpy (safety[2], "safety_2");
            strcat (safety[2], ".ori");
            strcpy (safety[3], "safety_3");
            strcat (safety[3], ".ori");
            strcpy (safety_addpar[0], "safety_0");
            strcat (safety_addpar[0], ".addpar");
            strcpy (safety_addpar[1], "safety_1");
            strcat (safety_addpar[1], ".addpar");
            strcpy (safety_addpar[2], "safety_2");
            strcat (safety_addpar[2], ".addpar");
            strcpy (safety_addpar[3], "safety_3");
            strcat (safety_addpar[3], ".addpar");
            
            for (i_img=0; i_img<n_img; i_img++)
            {
                for (i=0; i<nfix ; i++)
                {
                    pixel_to_metric (pix[i_img][i].x, pix[i_img][i].y,
                                     imx,imy, pix_x, pix_y,
                                     &crd[i_img][i].x, &crd[i_img][i].y,
                                     chfield);
                    crd[i_img][i].pnr = pix[i_img][i].pnr;
                }
                
                /* save data for special use of resection routine */
                if (examine == 4 && multi==0)
                {
                    printf ("try write resection data to disk\n");
                    /* point coordinates */
                    //sprintf (filename, "resect_%s.fix", img_name[i_img]);
                    sprintf (filename, "%s.fix", img_name[i_img]);
                    write_ori (Ex[i_img], I[i_img], G[i_img], img_ori[i_img]);
                    fp1 = fopen (filename, "w");
                    for (i=0; i<nfix; i++)
                        fprintf (fp1, "%3d  %10.5f  %10.5f  %10.5f\n",
                                 fix[i].pnr, fix[i].x, fix[i].y, fix[i].z);
                    fclose (fp1);
                    
                    /* metric image coordinates */
                    //sprintf (filename, "resect_%s.crd", img_name[i_img]);
                    sprintf (filename, "%s.crd", img_name[i_img]);
                    fp1 = fopen (filename, "w");
                    for (i=0; i<nfix; i++)
                        fprintf (fp1,
                                 "%3d  %9.5f  %9.5f\n", crd[i_img][i].pnr,
                                 crd[i_img][i].x, crd[i_img][i].y);
                    fclose (fp1);
                    
                    /* orientation and calibration approx data */
                    write_ori (Ex[i_img], I[i_img], G[i_img], "resect.ori0");
                    fp1 = fopen ("resect.ap0", "w");
                    fprintf (fp1, "%f %f %f %f %f %f %f",
                             ap[i_img].k1, ap[i_img].k2, ap[i_img].k3,
                             ap[i_img].p1, ap[i_img].p2,
                             ap[i_img].scx, ap[i_img].she);
                    fclose (fp1);
                    printf ("resection data written to disk\n");
                }
                
                
                /* resection routine */
                /* ================= */
                printf ("examine=%d\n",examine);
                printf("inside jw_ptv.c before orient_v3 I.yh=%f \n",I[i_img].yh);
                if (examine != 4)
                    orient_v3 (Ex[i_img], I[i_img], G[i_img], ap[i_img], mmp,
                               nfix, fix, crd[i_img],
                               &Ex[i_img], &I[i_img], &G[i_img], &ap[i_img], i_img);
                printf("inside jw_ptv.c after orient_v3 I.yh=%f \n",I[i_img].yh);
                /* ================= */
                
                
                /* resection with dumped datasets */
                if (examine == 4)
                {
                    
                    //printf("Resection with dumped datasets? (y/n)");
                    //scanf("%s",buf);
                    //if (buf[0] != 'y')	continue;
                    //strcpy (buf, "");
                    if (multi==0)	continue;
                    
                    /* read calibration frame datasets */
                    //sprintf (multi_filename[0],"img/calib_a_cam");
                    //sprintf (multi_filename[1],"img/calib_b_cam");
                    
                    fp1 = fopen_r ("parameters/multi_planes.par");
                    fscanf (fp1,"%d\n", &planes);
                    for(i=0;i<planes;i++) 
                        fscanf (fp1,"%s\n", &multi_filename[i]);
                    fclose(fp1);
                    
                    for (n=0, nfix=0, dump_for_rdb=0; n<10; n++)
                    {
                        //sprintf (filename, "resect.fix%d", n);
                        
                        sprintf (filename, "%s%d.tif.fix", multi_filename[n],i_img+1);
                        fp1 = fopen (filename, "r");
                        if (! fp1)	continue;
                        
                        printf("reading file: %s\n", filename);
                        printf ("reading dumped resect data #%d\n", n);
                        k = 0;
                        while ( fscanf (fp1, "%d %lf %lf %lf",
                                        &fix[nfix+k].pnr, &fix[nfix+k].x,
                                        &fix[nfix+k].y, &fix[nfix+k].z)
                               != EOF) k++;
                        fclose (fp1);
                        /* read metric image coordinates */
                        //sprintf (filename, "resect_%d.crd%d", i_img, n);
                        sprintf (filename, "%s%d.tif.crd", multi_filename[n],i_img+1);
                        printf("reading file: %s\n", filename);
                        fp1 = fopen (filename, "r");
                        for (i=nfix; i<nfix+k; i++)
                            fscanf (fp1, "%d %lf %lf",
                                    &crd[i_img][i].pnr,
                                    &crd[i_img][i].x, &crd[i_img][i].y);
                        nfix += k;
                        fclose (fp1);
                    }
                    
                    /* resection */
                    /*Beat Mai 2007*/
                    sprintf (filename, "raw%d.ori", i_img);
                    read_ori (&Ex[i_img], &I[i_img], &G[i_img], filename,
                        &(ap[i_img]), "addpar.raw", NULL);
                    
                    /* markus 14.05.2007 show coordinates combined */
                    for (i=0; i<nfix ; i++)			  
                    {
                        /* first crd->pix */
                        metric_to_pixel (crd[i_img][i].x, crd[i_img][i].y, imx,imy, pix_x,pix_y, &pix[i_img][i].x, &pix[i_img][i].y, chfield);
                        /*then draw crosses*/
                        //intx1 = (int) pix[i_img][i].x;
                        //inty1 = (int) pix[i_img][i].y;
                        
                        
                        //drawcross (interp, intx1, inty1, 3, i_img, "orange");
                        
                        
                    }
                    
                    
                    orient_v3 (Ex[i_img], I[i_img], G[i_img], ap[i_img], mmp,
                               nfix, fix, crd[i_img],
                               &Ex[i_img], &I[i_img], &G[i_img], &ap[i_img], i_img);
                    
                    
                    
                    
                    
                    
                    ///////////////////////////////////////////
                    
                    
                }
                
                
                /* save orientation and additional parameters */
                printf("inside jw_ptv.c I.yh=%f \n",I[i_img].yh);
                write_ori (Ex[i_img], I[i_img], G[i_img], img_ori[i_img]);
                fp1 = fopen( img_ori[i_img], "r" );
                if(fp1 != NULL) {
                    fclose(fp1);
                    read_ori (&sEx[i_img], &sI[i_img], &sG[i_img], 
                        img_ori[i_img], &(sap[i_img]), img_addpar0[i_img],
                        "addpar.raw");
                    
                    write_ori (sEx[i_img], sI[i_img], sG[i_img], safety[i_img]);
                    fp1 = fopen (safety_addpar[i_img], "w");
                    fprintf (fp1, "%f %f %f %f %f %f %f",
                             sap[i_img].k1, sap[i_img].k2, sap[i_img].k3,
                             sap[i_img].p1, sap[i_img].p2,
                             sap[i_img].scx, sap[i_img].she);
                    fclose (fp1);
                }
                else{
                    write_ori (Ex[i_img], I[i_img], G[i_img], safety[i_img]);
                    fp1 = fopen (safety_addpar[i_img], "w");
                    fprintf (fp1, "%f %f %f %f %f %f %f",
                             ap[i_img].k1, ap[i_img].k2, ap[i_img].k3,
                             ap[i_img].p1, ap[i_img].p2,
                             ap[i_img].scx, ap[i_img].she);
                    fclose (fp1);
                    
                }
                write_ori (Ex[i_img], I[i_img], G[i_img], img_ori[i_img]);
                fp1 = fopen (img_addpar[i_img], "w");
                fprintf (fp1, "%f %f %f %f %f %f %f",
                         ap[i_img].k1, ap[i_img].k2, ap[i_img].k3,
                         ap[i_img].p1, ap[i_img].p2,
                         ap[i_img].scx, ap[i_img].she);
                fclose (fp1);
            }
            /*      Tcl_Eval(interp, ".text delete 3");*/
            /*      Tcl_Eval(interp, ".text delete 1");*/
            /*      Tcl_Eval(interp, ".text insert 1 \"Orientation and self calibration \"");*/
            /*      Tcl_Eval(interp, ".text delete 2");*/
            //  if (examine != 4)
            /*         Tcl_Eval(interp, ".text insert 2 \"...done, sigma0 for each image -> \"");*/
            
            //  if (examine == 4 && multi==0)
            /*		 Tcl_Eval(interp, ".text insert 2 \"resection data written to disk \"");*/
            /*      Tcl_SetVar(interp, "tbuf", buf, TCL_GLOBAL_ONLY);*/
            /*      Tcl_Eval(interp, ".text insert 3 $tbuf");*/
            
            break;
            
        case 10: puts ("Orientation from particles"); strcpy(buf, "");
			
			strcpy (safety[0], "safety_0");
			strcat (safety[0], ".ori");
			strcpy (safety[1], "safety_1");
			strcat (safety[1], ".ori");
			strcpy (safety[2], "safety_2");
			strcat (safety[2], ".ori");
			strcpy (safety[3], "safety_3");
			strcat (safety[3], ".ori");
			strcpy (safety_addpar[0], "safety_0");
			strcat (safety_addpar[0], ".addpar");
			strcpy (safety_addpar[1], "safety_1");
			strcat (safety_addpar[1], ".addpar");
			strcpy (safety_addpar[2], "safety_2");
			strcat (safety_addpar[2], ".addpar");
			strcpy (safety_addpar[3], "safety_3");
			strcat (safety_addpar[3], ".addpar");
			
			for (i_img=0; i_img<n_img; i_img++)
			{
				
				/* read control point coordinates for man_ori points */
				
				
				fpp = fopen_r ("parameters/sequence.par");
				for (i=0; i<4; i++){
					fscanf (fpp, "%s\n", seq_name[i]);     /* name of sequence */
					//fscanf (fpp,"%d\n", &seq_first);
					//fscanf (fpp,"%d\n", &seq_last);
				}
				fclose (fpp);
				
				fpp = fopen_r ("parameters/shaking.par");
				fscanf (fpp,"%d\n", &seq_first); 
				fscanf (fpp,"%d\n", &seq_last);
				fscanf (fpp,"%d\n", &max_shake_points);
				fscanf (fpp,"%d\n", &max_shake_frames);
				fclose (fpp);
				
				/*  read from main parameter file  */
				fpp = fopen_r ("parameters/ptv.par");
				fscanf (fpp, "%d\n", &n_img);
				fclose (fpp);
				
				i=0;
				frameCount=0;
				currentFrame=0;
				step_shake=(int)((double)(seq_last-seq_first+1)/(double)max_shake_frames+0.5);
				printf("\nframe step size for camera %d is %d\n", i_img+1, step_shake);
				for (filenumber=seq_first+2; filenumber<seq_last+1-2; filenumber=filenumber+step_shake){//chnaged by Beat Feb 08
					
					if (filenumber < 10)        sprintf (filein, "res/rt_is.%1d", filenumber);
					else if (filenumber < 100)  sprintf (filein, "res/rt_is.%2d",  filenumber);
					else       sprintf (filein, "res/rt_is.%3d", filenumber);
					
					FILEIN = fopen (filein, "r");
					if (! FILEIN) printf("Can't open ascii file: %s\n", filein);
					/////////open target file(s)!
					/* read targets of each camera */
					
					if (filenumber < 10)        sprintf (filein_ptv, "res/ptv_is.%1d", filenumber);
					else if (filenumber < 100)  sprintf (filein_ptv, "res/ptv_is.%2d",  filenumber);
					else       sprintf (filein_ptv, "res/ptv_is.%3d", filenumber);
					
					
					// to only use quadruplets for shaking that can be linked
					FILEIN_ptv = fopen (filein_ptv, "r");
					if (! FILEIN_ptv) printf("Can't open ascii file: %s\n", filein_ptv);
					/////////open target file(s)!
					/* read targets of each camera */
					
                    nt4[3][i_img] = read_targets(t4[3][i_img], seq_name[i_img], \
                        filenumber); /* Was nt4[3][i] but that seems wrong */
					
					fscanf(FILEIN, "%d\n", &dumy); /* read # of 3D points on dumy */
					fscanf(FILEIN_ptv, "%d\n", &dumy); /* read # of 3D points on dumy */
					do{
						/*read dataset row by row, x,y,z and correspondences */
						a[0]=-1;a[1]=-1;a[2]=-1;a[3]=-1;
						if (n_img==4){
							fscanf(FILEIN, "%d %lf %lf %lf %d %d %d %d\n",
								   &dumy, &fix[i].x, &fix[i].y, &fix[i].z,
								   &a[0], &a[1], &a[2], &a[3]);
							fscanf(FILEIN_ptv, "%d %d %lf %lf %lf\n",
								   &prev, &next, &dummy_float, &dummy_float, &dummy_float);
						}
						if (n_img==3){
							fscanf(FILEIN, "%d %lf %lf %lf %d %d %d %d\n",
								   &dumy, &fix[i].x, &fix[i].y, &fix[i].z,
								   &a[0], &a[1], &a[2]);
							fscanf(FILEIN_ptv, "%d %d %lf %lf %lf\n",
								   &prev, &next, &dummy_float, &dummy_float, &dummy_float);
						}
						if (n_img==2){ // Alex's patch. 24.09.09. Working on Wesleyan data of 2 cameras only
							fscanf(FILEIN, "%d %lf %lf %lf %d %d %d %d\n",
								   &dumy, &fix[i].x, &fix[i].y, &fix[i].z,
								   &a[0], &a[1]);
							fscanf(FILEIN_ptv, "%d %d %lf %lf %lf\n",
								   &prev, &next, &dummy_float, &dummy_float, &dummy_float);
						}
						////////////auch pix lesen according a0,a1,a2,a3!!! 
						//fix[i].x>-25 && 
						//fix[i].x>-25 && 
						if( (a[i_img]>-1 && next>-1 && prev>-1 && i<max_shake_points && frameCount<max_shake_frames+1  )  ||
						   (a[0]>-1 && a[1]>-1 && a[2]>-1 && a[3]>-1 && next>-1 && prev>-1 && filenumber==seq_first+2 ) ){// OR ALLE QUADRUPLETS
							pix[i_img][i].x=t4[3][i_img][a[i_img]].x;
							pix[i_img][i].y=t4[3][i_img][a[i_img]].y;
							pix[i_img][i].pnr=i; 
							fix[i].pnr=i;
							
							i++;
							nfix =i;
							if(currentFrame<filenumber){
								currentFrame=filenumber;
								frameCount++;
							}
						}
						/*if (n_img==4){
						 if(a[0]>-1 && a[1]>-1 && a[2]>-1 && a[3]>-1 && next>-1 && prev>-1){
						 pix[i_img][i].x=t4[3][i_img][a[i_img]].x;
						 pix[i_img][i].y=t4[3][i_img][a[i_img]].y;
						 pix[i_img][i].pnr=i; 
						 fix[i].pnr=i;
						 
						 i++;
						 nfix =i;
						 }
						 }
						 if (n_img==3){
						 if(a[0]>-1 && a[1]>-1 && a[2]>-1 && next>-1 && prev>-1){
						 pix[i_img][i].x=t4[3][i_img][a[i_img]].x;
						 pix[i_img][i].y=t4[3][i_img][a[i_img]].y;
						 pix[i_img][i].pnr=i; 
						 fix[i].pnr=i;
						 
						 i++;
						 nfix =i;
						 }
						 }*/
						
					}while(!feof(FILEIN));
					fclose(FILEIN);
					
					
					
				}//end of loop through seq, but loop i_img still open
				if (frameCount==1){
					printf("Using %d linked points of %d frame for camera %d\n", nfix,frameCount, i_img+1);
				}
				else{
					printf("Using %d linked points of %d frames for camera %d\n", nfix,frameCount, i_img+1);    
				}	
				for (i=0; i<nfix ; i++)
				{
					pixel_to_metric (pix[i_img][i].x, pix[i_img][i].y,
									 imx,imy, pix_x, pix_y,
									 &crd[i_img][i].x, &crd[i_img][i].y,
									 chfield);
					crd[i_img][i].pnr = pix[i_img][i].pnr;
				}
				
				
				/* ================= */
				
				orient_v3 (Ex[i_img], I[i_img], G[i_img], ap[i_img], mmp,
						   nfix, fix, crd[i_img],
						   &Ex[i_img], &I[i_img], &G[i_img], &ap[i_img], i_img);
				
				/* ================= */
				
				
				/* save orientation and additional parameters */
				//make safety copy of ori files
				
				fp1 = fopen( img_ori[i_img], "r" );
				if(fp1 != NULL) {
					fclose(fp1);
					read_ori(&sEx[i_img], &sI[i_img], &sG[i_img],
                        img_ori[i_img], &(sap[i_img]), img_addpar0[i_img],
                        "addpar.raw");
					
					write_ori (sEx[i_img], sI[i_img], sG[i_img], safety[i_img]);
					fp1 = fopen (safety_addpar[i_img], "w");
					fprintf (fp1, "%f %f %f %f %f %f %f",
							 sap[i_img].k1, sap[i_img].k2, sap[i_img].k3,
							 sap[i_img].p1, sap[i_img].p2,
							 sap[i_img].scx, sap[i_img].she);
					fclose (fp1);
				}
				else{
					write_ori (Ex[i_img], I[i_img], G[i_img], safety[i_img]);
					fp1 = fopen (safety_addpar[i_img], "w");
					fprintf (fp1, "%f %f %f %f %f %f %f",
							 ap[i_img].k1, ap[i_img].k2, ap[i_img].k3,
							 ap[i_img].p1, ap[i_img].p2,
							 ap[i_img].scx, ap[i_img].she);
					fclose (fp1);
					
				}
				write_ori (Ex[i_img], I[i_img], G[i_img], img_ori[i_img]);
				fp1 = fopen (img_addpar[i_img], "w");
				fprintf (fp1, "%f %f %f %f %f %f %f",
						 ap[i_img].k1, ap[i_img].k2, ap[i_img].k3,
						 ap[i_img].p1, ap[i_img].p2,
						 ap[i_img].scx, ap[i_img].she);
				fclose (fp1);
			}
			/*
             Tcl_Eval(".text delete 3");
             Tcl_Eval(".text delete 1");
             Tcl_Eval(".text insert 1 \"Orientation from particles \"");
             Tcl_Eval(".text delete 2");
             if (examine != 4)
             Tcl_Eval(".text insert 2 \"...done, sigma0 for each image -> \"");
             if (examine == 4 && multi==0)
             Tcl_Eval(".text insert 2 \"resection data written to disk \"");
             Tcl_SetVar("tbuf", buf, TCL_GLOBAL_ONLY);
             Tcl_Eval(".text insert 3 $tbuf");
             */
			break;
        case 12: puts ("Orientation from dumbbells"); strcpy(buf, "");
			
            prepare_eval(n_img,&nfix); //goes and looks up what sequence is defined and takes all cord. from rt_is
            orient_v5 (n_img, nfix,&Ex, &I, &G, &ap);
			
            for(i_img=0;i_img<n_img;i_img++){
                write_ori (Ex[i_img], I[i_img], G[i_img], img_ori[i_img]);
                fp1 = fopen (img_addpar[i_img], "w");
                fprintf (fp1, "%f %f %f %f %f %f %f",
						 ap[i_img].k1, ap[i_img].k2, ap[i_img].k3,
						 ap[i_img].p1, ap[i_img].p2,
						 ap[i_img].scx, ap[i_img].she);
				fclose (fp1);
            }
			
            break;
            
    }
    
    //  return TCL_OK;
    return 0;
}

int sequence_proc_c  (int dumb_flag)
{
    int     i, j, ok, k, nslices=19, slicepos=0, pft_version = 3;
    //char    seq_ch[128], seq_name[4][128];
    // Tk_PhotoHandle img_handle;
    // Tk_PhotoImageBlock img_block;
    
    // z_cen_slice[19];
    int dumbbell=0;
    //  double dummy;
    
    seq_step_shake=1;
    fpp = fopen_r ("parameters/sequence.par");
    for (i=0; i<4; i++)
        fscanf (fpp, "%s\n", seq_name[i]);     /* name of sequence */
    fscanf (fpp,"%d\n", &seq_first);
    fscanf (fpp,"%d\n", &seq_last);
    fclose (fpp);
    
    
    //display = atoi(argv[1]); 
    //Beat Mai 2010 for dumbbell
    if (dumb_flag==3){
        dumbbell=1;
        display=0;
    }
    
    /* scanning ptv ************** */
    printf("\nObject volume is scanned in %d slices!\n", nslices);
    slicepos=0;
    /* read illuminated Volume */
    fpp = fopen_r ("parameters/criteria.par");
    fscanf (fpp, "%lf\n", &X_lay[0]);
    fscanf (fpp, "%lf\n", &Zmin_lay[0]);
    fscanf (fpp, "%lf\n", &Zmax_lay[0]);
    fscanf (fpp, "%lf\n", &X_lay[1]);
    fscanf (fpp, "%lf\n", &Zmin_lay[1]);
    fscanf (fpp, "%lf\n", &Zmax_lay[1]);
    fscanf (fpp, "%lf", &cnx);
    fscanf (fpp, "%lf", &cny);
    fscanf (fpp, "%lf", &cn);
    fscanf (fpp, "%lf", &csumg);
    fscanf (fpp, "%lf", &corrmin);
    fscanf (fpp, "%lf", &eps0);
    fclose (fpp);
    
    /* read illuminated layer data */
    if (dumbbell==1){
        fpp = fopen ("parameters/dumbbell.par", "r");
        if (fpp){
            fscanf (fpp, "%lf", &eps0);
            fscanf (fpp, "%lf", &seq_dummy);
            fscanf (fpp, "%lf", &seq_dummy);
            fscanf (fpp, "%lf", &seq_dummy);
            fscanf (fpp, "%d", &seq_step_shake);
            fclose (fpp);
        }
        else{
            fpp = fopen ("parameters/dumbbell.par", "w");
            fprintf(fpp,"%lf\n", 5.0);
            fprintf(fpp,"%lf\n", 46.5);
            fprintf(fpp,"%lf\n", 0.5);
            fprintf(fpp,"%lf\n", 2.);
            fprintf(fpp,"%d\n", 2);
            fprintf(fpp,"%d\n",500);
            fprintf(fpp,"\n\n");
            fprintf(fpp,"explanation for parameters:\n");
            fprintf(fpp,"\n");
            fprintf(fpp,"1: eps (mm)\n");
            fprintf(fpp,"2: dunbbell scale\n");
            fprintf(fpp,"3: gradient descent factor\n");
            fprintf(fpp,"4: weight for dumbbell penalty\n");
            fprintf(fpp,"5: step size through sequence\n");
            fprintf(fpp,"6: num iterations per click\n");
            fclose(fpp);
            eps0=10;
        }
    }
    
    
    mmp.nlay = 1;
    
    seq_zdim=Zmax_lay[0]-Zmin_lay[0];
    seq_slice_step= seq_zdim/nslices;
    seq_slicethickness=5.0;
    
    printf("\nzdim: %f, max: %f, min: %f, st: %f\n", seq_zdim,Zmax_lay[0], Zmin_lay[0], seq_slice_step);
    
    
    //for (j=0; j<nslices; j++)
    //	{
    //	  z_cen_slice[j]=Zmax_lay[0]-j*slice_step;
    
    //	}
    
    /* ************** */
    return 0;
}

int sequence_proc_loop_c  (int dumbbell,int i)
{
    double slice_step,slicethickness,zdim,dummy;
    
    int step_shake;
    int j,k,pft_version = 3,ok;
    
    slice_step=seq_slice_step;
    slicethickness=seq_slicethickness;
    zdim=seq_zdim;
    dummy=seq_dummy;
    step_shake=seq_step_shake;
    
    //printf("\nstep: %d, zslice[j]: %f, slicepos: %d\n", i);
    
    //	  Zmax_lay[0]= z_cen_slice[slicepos] - slicethickness/2.0;
    //	  Zmin_lay[0]= z_cen_slice[slicepos] + slicethickness/2.0;
    //	  Zmax_lay[1]= z_cen_slice[slicepos] - slicethickness/2.0;
    //	  Zmin_lay[1]= z_cen_slice[slicepos] + slicethickness/2.0;
    //printf("in sequence zslice[j]: %f, zmin0: %f, zmax0: %f\n",
    //z_cen_slice[slicepos], Zmax_lay[0],Zmin_lay[0] );
    
    //slicepos++; if (slicepos==nslices) {slicepos=0;}
    
    if (i < 10)             sprintf (seq_ch, "%1d", i);
    else if (i < 100)       sprintf (seq_ch, "%2d",  i);
    else       sprintf (seq_ch, "%3d",  i);
    
    for (j=0; j<n_img; j++)
	{
        sprintf (img_name[j], "%s%s", seq_name[j], seq_ch);
        sprintf (img_lp_name[j], "%s%s_lp", seq_name[j], seq_ch);
        sprintf (img_hp_name[j], "%s%s_hp", seq_name[j], seq_ch);
	}
    
    //Beat Mai 2010 for dumbbell
    if (dumbbell==0){
        if (chfield == 0)       sprintf (res_name, "res/rt_is.%s", seq_ch);
        else            sprintf (res_name, "res/rt_is.%s_%1d", seq_ch, chfield);
    }
    else{
        if (chfield == 0)       sprintf (res_name, "res/db_is.%s", seq_ch);
        else            sprintf (res_name, "res/db_is.%s_%1d", seq_ch, chfield);
    }
    sprintf (buf, "\nImages:");
    for (j=0; j<n_img; j++) sprintf (buf, "%s  %s", buf, img_name[j]);
    puts (buf);
    
    /* calling function for each sequence-n-tupel */
    
    /* read and display original images */
    
    for (k=0; k<n_img; k++)
	{
        /* reading */
        //   read_image (interp, img_name[k], img[k]);
        
        /*	  if (display) {*/
        /*	    img_handle = Tk_FindPhoto( interp, "temp");*/
        /*	    Tk_PhotoGetImage (img_handle, &img_block);*/
        /*	    tclimg2cimg (interp, img[k], &img_block);*/
        /*	    sprintf(buf, "newimage %d", k+1);*/
        /*	    Tcl_Eval(interp, buf);*/
        /*	  }*/
	}
    
    /*  read pft version  */
    /* added by Alex for external detection procedure, 19.4.10 */
    fpp = fopen ("parameters/pft_version.par", "r");
    if (fpp)
    {
        fscanf (fpp, "%d\n", &pft_version);
        pft_version=pft_version+3;
        fclose (fpp);
    }
    
    if (hp_flag) {
        pre_processing_c ();
        puts("\nHighpass switched on\n");
    } else { puts("\nHighpass switched off\n"); }
    /*      if (display) {Tcl_Eval(interp, "update idletasks");}*/
    /**************************************************************************************/
    /* pft_version = 4 means external detection, Alex, 19.4.10 */
    
    if ( pft_version == 4) { 
		for (k=0; k<n_img; k++) {
            num[k] = read_targets(pix[k], seq_name[k], i);
            nt4[3][k] = num[k]; // Until we get to globals removal fulltime.
            
            /* proper sort of targets in y-direction for later binary search */
            /* and for dimitris' tracking */
            quicksort_target_y (pix[k], num[k]);
            /* reorganize target numbers */
            for (j=0; j<num[k]; j++)  pix[k][j].pnr = j;
        }
    } 
    /***************************************************************************************/
    else {
		detection_proc_c (); // added i to the detection_proc_c to get 'filenumber' for external API, Alex, 19.04.10
    }
    
    /*	  if (display) {Tcl_Eval(interp, "update idletasks");}*/
    correspondences_proc_c ();
    /*      if (display) {Tcl_Eval(interp, "update idletasks");}*/
    if(n_img>1){
		determination_proc_c (dumbbell);}
    
    
    /* delete unneeded files */
    
    for (j=0; j<n_img; j++)
	{
        ok = remove (img_lp_name[j]);
        ok = remove (img_hp_name[j]);
	}
    
    /* reset of display flag */
    display = 1;
    
    return 0;
}


int determination_proc_c (int dumbbell)
{
    int  	i, j, n,dummy;
    int  	p[4];
    double  x[4], y[4], X,Y,Z;
    double  Zlo = 1e20, Zhi = -1e20;
    //int dumbbell=0,i1,i2;
    int i1,i2;
    double x1,y1,z1,x2,y2,z2,dist,mx,my,mz,nx,ny,nz;
    int a1[4],a2[4],checksum_1,checksum_2;
    
    
    puts ("Determinate");
    
    sprintf (buf, "Point positioning (mid_point in 3d)");
    /*  Tcl_SetVar(interp, "tbuf", buf, TCL_GLOBAL_ONLY);*/
    /*  Tcl_Eval(interp, ".text delete 2");*/
    /*  Tcl_Eval(interp, ".text insert 2 $tbuf");
     */
    
    
    /* Beat Mai 2007 to set the variable examine for mulit-plane calibration*/
    fp1 = fopen_r ("parameters/examine.par");
    fscanf (fp1,"%d\n", &dummy);
    fclose (fp1);
    if (dummy==1){
        examine=4;
    }
    else{
        examine=0;
    }
    //////////////////////////////////
    
    
    fp1 = fopen (res_name, "w");
    
    if ( ! fp1)
    {
        sprintf(res_name,"res/dt_lsq");
        fp1 = fopen (res_name, "w");
    }
    if ( ! fp1)
    {
        printf ("cannot find dir: res,  data written to dt_lsq in same dir\n");
        sprintf (res_name, "dt_lsq");
        fp1 = fopen (res_name, "w");
    }
    /* create dump file for rdb */
    if (examine == 4)
    {
        /* create filename for dumped dataset */
        sprintf (res_name, "dump_for_rdb");
        printf ("dataset dumped into %s\n", res_name);
        fp2 = fopen (res_name, "w");
        
        /* write # of points to file */
        fprintf (fp2, "%d\n", match);
    }
    /* first line to be updated in res_name file */
    fprintf (fp1, "%4d\n", match);
    /* least squares determination for triplets */
    
    rmsX = 0; rmsY = 0; rmsZ = 0;	mean_sigma0 = 0;
    
    for (i=0; i<match; i++)
    {
        for (j=0; j<4; j++)
            if (con[i].p[j] >= 0)	p[j] = geo[j][con[i].p[j]].pnr;
            else		       	p[j] = -1;
        
        for (j=0, n=0; j<4; j++)
        {
            if (p[j] > -1)
            {
                x[j] = crd[j][p[j]].x;	y[j] = crd[j][p[j]].y;
                n++;
            }
            else
            {
                x[j] = -1e10;	y[j] = -1e10;
                if (p[j] == -2)	n = -100;
            }
        }
        
        /* take only points which are matched in all images */
        /* or triplets/quadruplets which result from object model */
        /* e.g.: quad -> n=4; model triplet -> n=3; model pair -> n=2;
         unrestricted triplet -> n<0; unrestricted pair -> n<0 */
        /*     if (n_img > 2  &&  n < 3)	continue; */
        
        /* ################################# */
        /* take only points which are matched in all images */
        /* or triplets/quadruplets which result from object model */
        /* e.g.: quad -> n=4; model triplet -> n=3; model pair -> n=2;
         unrestricted triplet -> n<0; unrestricted pair -> n<0 */
        if ((n_img > 2 && num[0]>64 && num[1]>64 && num[2]>64 && num[3]>64)
            &&  n < 3)	continue;
        
        /* hack due to problems with approx in det_lsq: */
        X = 0.0; Y = 0.0; Z = (Zmin_lay[0]+Zmax_lay[0])/2.0;
        for (j=0; j<n_img; j++) { X += Ex[j].x0; Y += Ex[j].y0; }
        X /= n_img; Y /= n_img;
        /* ******************************** */
        
        //det_lsq_old (Ex, I, ap, mmp,
        //     x[0], y[0], x[1], y[1], x[2], y[2], x[3], y[3], &X, &Y, &Z);
        
        
        det_lsq_3d (Ex, I, G, ap, mmp,
                    x[0], y[0], x[1], y[1], x[2], y[2], x[3], y[3], &X, &Y, &Z);
        
        
        /* write a sequential point number,
         sumg, if the point was used, and the 3D coordinates */
        fprintf (fp1, "%4d", i+1);
        
        
        /*
         if (p[0] > -1)	fprintf (fp1, "  %4d", pix[0][p[0]].sumg);
         else			fprintf (fp1, "   %4d", -1);
         if (p[1] > -1)	fprintf (fp1, "  %4d", pix[1][p[1]].sumg);
         else			fprintf (fp1, "  %4d", -1);
         if (p[2] > -1)	fprintf (fp1, "  %4d", pix[2][p[2]].sumg);
         else			fprintf (fp1, "  %4d", -1);
         if (p[3] > -1)	fprintf (fp1, "  %4d", pix[3][p[3]].sumg);
         else			fprintf (fp1, "  %4d", -1);
         */
        
        fprintf (fp1, " %9.3f %9.3f %9.3f", X, Y, Z);
        if (p[0] > -1)	fprintf (fp1, " %4d", pix[0][p[0]].pnr);
        else			fprintf (fp1, " %4d", -1);
        if (p[1] > -1)	fprintf (fp1, " %4d", pix[1][p[1]].pnr);
        else			fprintf (fp1, " %4d", -1);
        if (p[2] > -1)	fprintf (fp1, " %4d", pix[2][p[2]].pnr);
        else			fprintf (fp1, " %4d", -1);
        if (p[3] > -1)	fprintf (fp1, " %4d\n", pix[3][p[3]].pnr);
        else			fprintf (fp1, " %4d\n", -1);
        
        /* write data as new points to dump for rdb */
        if (examine == 4)
        {
            fprintf (fp2, "%d %10.3f %10.3f %10.3f   %d    ", i, X, Y, Z, 3);
            for (j=0; j<n_img; j++)
                if (x[j] != -1e10)
                    fprintf (fp2, "%4d %8.5f %8.5f    ", i, x[j], y[j]);
                else
                    fprintf (fp2, "%4d %8.5f %8.5f    ", -999, x[j], y[j]);
            fprintf (fp2, "\n");
        }
        
        if (Z < Zlo)  Zlo = Z;   if (Z > Zhi)  Zhi = Z;
    }
    
    if (examine == 4) fclose (fp2);
    fclose (fp1);
    
    //Beat Mai 2010: now we should open the file db_is.* again, check
    //               if it has exactly two points, rescale them, write them again and close the file.
    
    /*  if (atoi(argv[1])==3){*/
    /*      dumbbell=1;*/
    /*	  display=0;*/
    /*  }*/  //Denis
    if (dumbbell==1) {display=0;} //Denis
    
    if (dumbbell==1){
        fpp = fopen ("parameters/dumbbell.par", "r");
        if (fpp){
            fscanf (fpp, "%lf", &eps0);
            fscanf (fpp, "%lf", &db_scale);
            fclose (fpp);
        }
        
        fpp = fopen (res_name, "r");
        fscanf (fpp, "%d\n", &match);
        if(match==2){
            fscanf(fpp, "%d %lf %lf %lf %d %d %d %d\n",
                   &i1, &x1, &y1, &z1,
                   &a1[0], &a1[1], &a1[2], &a1[3]);
            fscanf(fpp, "%d %lf %lf %lf %d %d %d %d\n",
                   &i2, &x2, &y2, &z2,
                   &a2[0], &a2[1], &a2[2], &a2[3]);
            //now adapt x,y,z
            /*dist=pow(pow(x2-x1,2.)+pow(y2-y1,2.)+pow(z2-z1,2.),0.5);
             mx=0.5*(x1+x2);
             my=0.5*(y1+y2);
             mz=0.5*(z1+z2);
             nx=(x2-x1)/dist;
             ny=(y2-y1)/dist;
             nz=(z2-z1)/dist;
             x1=mx-0.5*db_scale*nx;
             x2=mx+0.5*db_scale*nx;
             y1=my-0.5*db_scale*ny;
             y2=my+0.5*db_scale*ny;
             z1=mz-0.5*db_scale*nz;
             z2=mz+0.5*db_scale*nz;*/
            
            //check if reasonable
            /*dist=pow(pow(x2-x1,2.)+pow(y2-y1,2.)+pow(z2-z1,2.),0.5);
             if (fabs(dist-38)>1){
			 match=0;
             }*/
            
            //check if all quadruplets or triplets
            checksum_1=0;
            checksum_2=0;
            for(j=0;j<4;j++){
                if(a1[1]<0){
                    checksum_1++;
                }
                if(a2[1]<0){
                    checksum_2++;
                }
            }
            if(checksum_1>1 || checksum_2>1){
                match=0;
            }
            //end of check if all quadruplets or triplets
        }
        else{
            match=0;
        }
        fclose (fpp);
        fpp = fopen (res_name, "w");
        if(match==2){
            fprintf (fpp, "%4d\n", match);
            fprintf (fpp, " %4d %9.3f %9.3f %9.3f %4d %4d %4d %4d\n", i1,x1,y1,z1,a1[0],a1[1],a1[2],a1[3]);
            fprintf (fpp, " %4d %9.3f %9.3f %9.3f %4d %4d %4d %4d\n", i2,x2,y2,z2,a2[0],a2[1],a2[2],a2[3]);
        }
        else{
            fprintf (fpp, "%4d\n", 0);
        }
        fclose (fpp);
        
    }
    //end of dumbbell treatment
    
    rmsX = sqrt(rmsX/match); rmsY = sqrt(rmsY/match); rmsZ = sqrt(rmsZ/match);
    mean_sigma0 = sqrt (mean_sigma0/match);
    
    sprintf (buf, "Match: %d, => rms = %4.2f micron, rms_x,y,z = %5.3f/%5.3f/%5.3f mm", match, mean_sigma0*1000, rmsX, rmsY, rmsZ);
    puts (buf);
    /*  Tcl_SetVar(interp, "tbuf", buf, TCL_GLOBAL_ONLY);*/
    /*  Tcl_Eval(interp, ".text delete 3");*/
    /*  Tcl_Eval(interp, ".text insert 3 $tbuf");*/
    
    /* sort coordinates for binary search in epi line segment drawing */
    for (i=0; i<n_img; i++)  quicksort_coord2d_x (geo[0], num[0]);
    
    puts ("Determinate done\n");
    
    return 0;
    
}


