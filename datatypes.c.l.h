/* Generated by           cobc 3.3-dev.0 */
/* Generated from         datatypes.cob */
/* Generated at           dez 28 2023 09:57:53 */
/* GnuCOBOL build date    Dec 26 2023 19:54:15 */
/* GnuCOBOL package date  Aug 22 2023 07:38:54 UTC */
/* Compile command        cobc -g -fsource-location -ftraceall -v -O0 -x datatypes.cob */

/* Program local variables for 'datatypes' */

/* Module initialization indicator */
static unsigned int	initialized = 0;

/* Module structure pointer */
static cob_module	*module = NULL;

/* Global variable pointer */
cob_global		*cob_glob_ptr;


/* LINKAGE SECTION (Items not referenced by USING clause) */
static unsigned char	*b_9 = NULL;  /* XML-NAMESPACE */
static unsigned char	*b_10 = NULL;  /* XML-NAMESPACE-PREFIX */
static unsigned char	*b_11 = NULL;  /* XML-NNAMESPACE */
static unsigned char	*b_12 = NULL;  /* XML-NNAMESPACE-PREFIX */
static unsigned char	*b_13 = NULL;  /* XML-NTEXT */
static unsigned char	*b_14 = NULL;  /* XML-TEXT */


/* Call parameters */
cob_field		*cob_procedure_params[1];

/* Perform frame stack */
struct cob_frame	*frame_overflow;
struct cob_frame	*frame_ptr;
struct cob_frame	frame_stack[255];


/* Data storage */
static int	b_2;	/* RETURN-CODE */
static cob_u8_t	b_17[93] __attribute__((aligned));	/* numeric-data */
static cob_u8_t	b_43[12] __attribute__((aligned));	/* floating-data */
static cob_u8_t	b_46[17] __attribute__((aligned));	/* special-data */
static cob_u8_t	b_52[101] __attribute__((aligned));	/* alphanumeric-data */
static cob_u8_t	b_59[160] __attribute__((aligned));	/* national-data */

/* End of local data storage */


/* Fields (local) */
static cob_field f_17	= {93, b_17, &a_37};	/* numeric-data */
static cob_field f_18	= {5, b_17, &a_7};	/* disp1 */
static cob_field f_19	= {5, b_17 + 5, &a_8};	/* disp-u */
static cob_field f_20	= {4, b_17 + 10, &a_9};	/* dispp */
static cob_field f_21	= {4, b_17 + 14, &a_10};	/* dispp-u */
static cob_field f_22	= {4, b_17 + 18, &a_11};	/* disppp */
static cob_field f_23	= {4, b_17 + 22, &a_12};	/* disppp-u */
static cob_field f_24	= {4, b_17 + 26, &a_13};	/* bin */
static cob_field f_25	= {4, b_17 + 30, &a_14};	/* bin-u */
static cob_field f_26	= {3, b_17 + 34, &a_15};	/* cmp3 */
static cob_field f_27	= {3, b_17 + 37, &a_16};	/* cmp3-u */
static cob_field f_28	= {4, b_17 + 40, &a_2};	/* cmp5 */
static cob_field f_29	= {4, b_17 + 44, &a_17};	/* cmp5-u */
static cob_field f_30	= {3, b_17 + 48, &a_18};	/* cmp6 */
static cob_field f_31	= {3, b_17 + 51, &a_19};	/* cmpx */
static cob_field f_32	= {3, b_17 + 54, &a_20};	/* cmpx-u */
static cob_field f_33	= {3, b_17 + 57, &a_19};	/* cmpn */
static cob_field f_34	= {3, b_17 + 60, &a_20};	/* cmpn-u */
static cob_field f_35	= {1, b_17 + 63, &a_21};	/* chr */
static cob_field f_36	= {1, b_17 + 64, &a_22};	/* chr-u */
static cob_field f_37	= {2, b_17 + 65, &a_23};	/* shrt */
static cob_field f_38	= {2, b_17 + 67, &a_24};	/* shrt-u */
static cob_field f_39	= {4, b_17 + 69, &a_25};	/* long */
static cob_field f_40	= {4, b_17 + 73, &a_26};	/* long-u */
static cob_field f_41	= {8, b_17 + 77, &a_27};	/* dble */
static cob_field f_42	= {8, b_17 + 85, &a_28};	/* dble-u */
static cob_field f_43	= {12, b_43, &a_37};	/* floating-data */
static cob_field f_44	= {8, b_43, &a_29};	/* dbl */
static cob_field f_45	= {4, b_43 + 8, &a_30};	/* flt */
static cob_field f_46	= {17, b_46, &a_37};	/* special-data */
static cob_field f_47	= {1, b_46, &a_31};	/* r2d2 */
static cob_field f_48	= {4, b_46 + 1, &a_32};	/* point */
static cob_field f_49	= {4, b_46 + 5, &a_32};	/* ppoint */
static cob_field f_50	= {4, b_46 + 9, &a_33};	/* idx */
static cob_field f_51	= {4, b_46 + 13, &a_34};	/* hnd */
static cob_field f_52	= {101, b_52, &a_37};	/* alphanumeric-data */
static cob_field f_53	= {36, b_52, &a_35};	/* alpnum */
static cob_field f_54	= {36, b_52 + 36, &a_35};	/* alpha */
static cob_field f_55	= {7, b_52 + 72, &a_3};	/* edit-num1 */
static cob_field f_56	= {7, b_52 + 79, &a_4};	/* edit-num2 */
static cob_field f_57	= {7, b_52 + 86, &a_5};	/* edit-num3 */
static cob_field f_58	= {8, b_52 + 93, &a_6};	/* edit-num4 */
static cob_field f_59	= {160, b_59, &a_37};	/* national-data */
static cob_field f_60	= {72, b_59, &a_36};	/* nat */
static cob_field f_62	= {14, b_59 + 102, &a_3};	/* net-num1 */
static cob_field f_63	= {14, b_59 + 116, &a_4};	/* net-num2 */
static cob_field f_64	= {14, b_59 + 130, &a_5};	/* net-num3 */
static cob_field f_65	= {16, b_59 + 144, &a_6};	/* net-num4 */

/* End of fields */
