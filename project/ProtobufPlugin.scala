import sbt._
import Jni.Keys._
import Keys._


object ProtobufPlugin {
  val protobufConfig = config("protobuf")

  object Keys {
    val includePaths = TaskKey[Seq[File]]("protobuf-include-paths", "The paths that contain *.proto dependencies.")
    val protoc = SettingKey[String]("protobuf-protoc", "The path+name of the protoc executable.")
    val externalIncludePath = SettingKey[File]("protobuf-external-include-path", "The path to which protobuf:library-dependencies are extracted and which is used as protobuf:include-path for protoc")
    val generatedTargets = SettingKey[Seq[(File,String)]]("protobuf-generated-targets", "Targets for protoc: target directory and glob for generated source files")
    val generate = TaskKey[Seq[File]]("protobuf-generate", "Compile the protobuf sources.")
    val unpackDependencies = TaskKey[UnpackedDependencies]("protobuf-unpack-dependencies", "Unpack dependencies.")
    val protocOptions = SettingKey[Seq[String]]("protobuf-protoc-options", "Additional options to be passed to protoc")
  }

  import Keys._

  val settings: Seq[Setting[_]] = inConfig(protobufConfig)(Seq[Setting[_]](
    sourceDirectory <<= (sourceDirectory in Compile) { _ / "protobuf" },
    sourceDirectories <<= sourceDirectory apply (_ :: Nil),
    nativeSource <<= (managedNativeSourceDirectories in Compile) { _ / "compiled_protobuf" },
    javaSource <<= (sourceManaged in Compile) { _ / "compiled_protobuf" },
    externalIncludePath <<= target(_ / "protobuf_external"),
    protoc := "protoc",
    version := "2.5.0",

    generatedTargets := Nil,
    generatedTargets <+= (javaSource in protobufConfig)((_, "*.java")), // add javaSource to the list of patterns
    generatedTargets <+= (nativeSource in protobufConfig)((_, "*.pb.cpp")),

    protocOptions := Nil,
    protocOptions <++= (generatedTargets in protobufConfig){ generatedTargets =>
      val java_out =
        generatedTargets.find(_._2.endsWith(".java")) match {
          case Some(targetForJava) => Seq("--java_out=%s".format(targetForJava._1.absolutePath))
          case None => Nil
        }
      val cpp_out =
        generatedTargets.find(_._2.endsWith(".pb.cpp")) match {
          case Some(targetForCpp) => Seq("--cpp_out=%s".format(targetForCpp._1.absolutePath))
          case None => Nil
        }
      java_out ++ cpp_out
    },

    managedClasspath <<= (classpathTypes, update) map { (ct, report) =>
      Classpaths.managedJars(protobufConfig, ct, report)
    },

    unpackDependencies <<= unpackDependenciesTask,

    includePaths <<= (sourceDirectory in protobufConfig) map (identity(_) :: Nil),
    includePaths <+= externalIncludePath map (identity(_)),

    generate <<= sourceGeneratorTask.dependsOn(unpackDependencies)

  )) ++ Seq[Setting[_]](
    sourceGenerators in Compile <+= generate in protobufConfig,
    cleanFiles <++= (generatedTargets in protobufConfig){_.map{_._1}},
    cleanFiles <+= (externalIncludePath in protobufConfig),
    managedSourceDirectories in Compile <++= (generatedTargets in protobufConfig){_.map{_._1}},
    libraryDependencies <+= (version in protobufConfig)("com.google.protobuf" % "protobuf-java" % _),
    ivyConfigurations += protobufConfig,

    extraHeadersPath := (managedNativeSourceDirectories in Compile).value / "compiled_protobuf",
    jniSourceFiles <++= (generatedTargets in protobufConfig){ generatedTargets =>
      generatedTargets.find(_._2.endsWith(".pb.cpp")) match {
        case Some(targetForCpp) => (targetForCpp._1 ** targetForCpp._2).get
        case None => Nil
      }
    }
  )

  case class UnpackedDependencies(dir: File, files: Seq[File])

  private def executeProtoc(protocCommand: String, schemas: Set[File], includePaths: Seq[File], protocOptions: Seq[String], log: Logger) =
    try {
      val incPath = includePaths.map("-I" + _.absolutePath)
      val proc = Process(
        protocCommand,
        incPath ++ protocOptions ++ schemas.map(_.absolutePath)
      )
      proc ! log
    } catch { case e: Exception =>
      throw new RuntimeException("error occured while compiling protobuf files: %s" format(e.getMessage), e)
    }


  private def compile(protocCommand: String, schemas: Set[File], includePaths: Seq[File], protocOptions: Seq[String], generatedTargets: Seq[(File, String)], log: Logger) = {
    val generatedTargetDirs = generatedTargets.map(_._1)

    generatedTargetDirs.foreach(_.mkdirs())

    log.info("Compiling %d protobuf files to %s".format(schemas.size, generatedTargetDirs.mkString(",")))
    log.debug("protoc options:")
    protocOptions.map("\t"+_).foreach(log.debug(_))
    schemas.foreach(schema => log.info("Compiling schema %s" format schema))

    val exitCode = executeProtoc(protocCommand, schemas, includePaths, protocOptions, log)
    if (exitCode != 0)
      sys.error("protoc returned exit code: %d" format exitCode)

    log.info("Compiling protobuf")
    generatedTargetDirs.foreach{ dir =>
      log.info("Protoc target directory: %s".format(dir.absolutePath))
    }

    (generatedTargets.flatMap{ot => (ot._1 ** ot._2).get}).toSet
  }

  private def unpack(deps: Seq[File], extractTarget: File, log: Logger): Seq[File] = {
    IO.createDirectory(extractTarget)
    deps.flatMap { dep =>
      val seq = IO.unzip(dep, extractTarget, "*.proto").toSeq
      if (!seq.isEmpty) log.debug("Extracted " + seq.mkString("\n * ", "\n * ", ""))
      seq
    }
  }

  private def sourceGeneratorTask =
    (streams, sourceDirectories in protobufConfig, includePaths in protobufConfig, protocOptions in protobufConfig, generatedTargets in protobufConfig, protoc) map {
    (out, srcDirs, includePaths, protocOpts, otherTargets, protocCommand) =>
      val schemas = srcDirs.toSet[File].flatMap(srcDir => (srcDir ** "*.proto").get.map(_.getAbsoluteFile))
      val cachedCompile = FileFunction.cached(out.cacheDirectory / "protobuf", inStyle = FilesInfo.lastModified, outStyle = FilesInfo.exists) { (in: Set[File]) =>
        compile(protocCommand, schemas, includePaths, protocOpts, otherTargets, out.log)
      }
      cachedCompile(schemas).toSeq
  }

  private def unpackDependenciesTask = (streams, managedClasspath in protobufConfig, externalIncludePath in protobufConfig) map {
    (out, deps, extractTarget) =>
      val extractedFiles = unpack(deps.map(_.data), extractTarget, out.log)
      UnpackedDependencies(extractTarget, extractedFiles)
  }
}
