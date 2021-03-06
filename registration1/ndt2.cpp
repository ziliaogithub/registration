#include <iostream>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/registration/ndt.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <boost/thread/thread.hpp>
#include <pcl/filters/voxel_grid.h>
#include <math.h>
int
main(int argc, char** argv)
{

    pcl::PointCloud<pcl::PointXYZ>::Ptr target_cloud(new pcl::PointCloud<pcl::PointXYZ>);
    if (pcl::io::loadPCDFile<pcl::PointXYZ>("bunny.pcd", *target_cloud) == -1)
    {
        PCL_ERROR("Couldn't read file bunny.pcd \n");
        return (-1);
    }
    std::cout << "Loaded " << target_cloud->size() << " data points from bunny.pcd" << std::endl;

    pcl::PointCloud<pcl::PointXYZ>::Ptr input_cloud(new pcl::PointCloud<pcl::PointXYZ>);
    if (pcl::io::loadPCDFile<pcl::PointXYZ>("bunny_rotated.pcd", *input_cloud) == -1)
    {
        PCL_ERROR("Couldn't read file bunny_rotated.pcd \n");
        return (-1);
    }
    std::cout << "Loaded " << input_cloud->size() << " data points from bunny_rotated.pcd" << std::endl;

    //将输入的扫描过滤到原始尺寸的大概10%以提高匹配的速度。
    pcl::PointCloud<pcl::PointXYZ>::Ptr filtered_cloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::VoxelGrid<pcl::PointXYZ> voxel_filter;
    voxel_filter.setLeafSize(0.01, 0.01, 0.01);
    voxel_filter.setInputCloud(input_cloud);
    voxel_filter.filter(*filtered_cloud);
    std::cout << "Filtered cloud contains " << filtered_cloud->size()
        << " data points from bunny_rotated.pcd" << std::endl;
    //初始化正态分布变换（NDT）
    pcl::NormalDistributionsTransform<pcl::PointXYZ, pcl::PointXYZ> ndt;
    //设置依赖尺度NDT参数
    //为终止条件设置最小转换差异
    ndt.setTransformationEpsilon(0.05);
    //为More-Thuente线搜索设置最大步长
    ndt.setStepSize(0.08);
    //设置NDT网格结构的分辨率（VoxelGridCovariance）
    ndt.setResolution(0.6);
    //设置匹配迭代的最大次数
    ndt.setMaximumIterations(40);
    // 设置要配准的点云
    ndt.setInputSource(filtered_cloud);
    //设置点云配准目标
    ndt.setInputTarget(target_cloud);
/*
    //设置使用机器人测距法得到的初始对准估计结果
    Eigen::AngleAxisf init_rotation(0.6931, Eigen::Vector3f::UnitZ());
    Eigen::Translation3f init_translation(1.79387, 0.720047, 0);
    Eigen::Matrix4f init_guess = (init_translation * init_rotation).matrix();
*/
    Eigen::Matrix4f init_guess;
    init_guess<<0.815905,0.569306,0.100948,-0.571533,
               -0.569183,0.821554,-0.0328553,0.399136,
               -0.101639,-0.030651,0.994349,0.0703133,
                0,0,0,1;

    //计算需要的刚体变换以便将输入的点云匹配到目标点云
    pcl::PointCloud<pcl::PointXYZ>::Ptr output_cloud(new pcl::PointCloud<pcl::PointXYZ>);
    ndt.align(*output_cloud, init_guess);
    //ndt.align(*output_cloud);
    std::cout << "Normal Distributions Transform has converged:" << ndt.hasConverged()
        << " score: " << ndt.getFitnessScore() << std::endl;
    //计算误差
    Eigen::Matrix4f result_mastrix;
    result_mastrix=ndt.getFinalTransformation();
    std::cout<<result_mastrix<<endl;
    double error;
    error=1-fabs(atan(result_mastrix(1,0)/result_mastrix(0,0)))/(M_PI/5);
    cout<<"error: "<<error<<endl;
    //使用创建的变换对未过滤的输入点云进行变换
    pcl::transformPointCloud(*input_cloud, *output_cloud, ndt.getFinalTransformation());
    //保存转换的输入点云
    pcl::io::savePCDFileASCII("bunny_transformed_ndt.pcd", *output_cloud);
    // 初始化点云可视化界面
    boost::shared_ptr<pcl::visualization::PCLVisualizer>
        viewer_final(new pcl::visualization::PCLVisualizer("3D Viewer"));
    viewer_final->setBackgroundColor(0, 0, 0);
    //对目标点云着色（红色）并可视化
    pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ>
        target_color(target_cloud, 255, 0, 0);
    viewer_final->addPointCloud<pcl::PointXYZ>(target_cloud, target_color, "target cloud");
    viewer_final->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
        1, "target cloud");
    //对转换后的点云着色（绿色）并可视化
    pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ>
        output_color(output_cloud, 0, 255, 0);
    viewer_final->addPointCloud<pcl::PointXYZ>(output_cloud, output_color, "output cloud");
    viewer_final->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
        1, "output cloud");
    //对原始点云着蓝色
    pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ>
        input_color(output_cloud, 0, 0, 255);
    viewer_final->addPointCloud<pcl::PointXYZ>(input_cloud, input_color, "input cloud");
    viewer_final->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
        1, "input cloud");
    // 启动可视化
    //viewer_final->addCoordinateSystem(1.0);
    //viewer_final->spin();
    //等待直到可视化窗口关闭。
    while (!viewer_final->wasStopped())
    {
        viewer_final->spinOnce(100);
        boost::this_thread::sleep(boost::posix_time::microseconds(100000));
    }
    return (0);
}
