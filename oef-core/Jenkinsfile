pipeline {
  
  agent {
    docker {
      image 'gcr.io/organic-storm-201412/fetch-oefcore-develop'
    }
  }

  stages {
    
    stage('Build') {
      steps {
        sh './scripts/build-cmake.sh'
      }
    }

    stage('Test') {
      steps {
        sh './scripts/build-test.sh'
      }
    }
    
    stage('ci-tools') {
    parallel {
      stage('Format') {
        steps {
          sh './scripts/code-format-apply.sh ./'
        }
      }

      stage('Lint') {
        steps {
          sh './scripts/code-lint-apply.sh ./ ./build/'
        }
      }

      stage('Static-analyze-clang') {
        steps {
          sh './scripts/code-static-analyze-clang.sh ./ ./build/'
        }
      }

      stage('Static-analyze-cppcheck') {
        steps {
          sh './scripts/code-static-analyze-cppcheck.sh ./'
        }
      }

    } // parallel
    } // ci-tools

  } // stages

} // pipeline

/* Jenkins file from 
 * https://github.com/uvue-git/fetch-ledger/blob/develop/Jenkinsfile 
 */

//  agent none
//
//  stages {
//
//    stage('Builds & Tests') {
//      parallel {
//
//        stage('Basic Checks') {
//          agent {
//            docker {
//              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
//            }
//          }
//
//          stages {
//            stage('License Checks') {
//              steps {
//                sh './scripts/check-license-header.py'
//              }
//            }
//            stage('Style check') {
//              steps {
//                sh './scripts/apply-style.py -w -a'
//              }
//            }
//          }
//        } // basic checks
//
//        stage('Clang 6 Debug') {
//          agent {
//            docker {
//              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
//            }
//          }
//
//          stages {
//            stage('Debug Build') {
//              steps {
//                sh './scripts/ci-tool.py -B Debug'
//              }
//            }
//            stage('Debug Unit Tests') {
//              steps {
//                sh './scripts/ci-tool.py -T Debug'
//              }
//            }
//            stage('Static Analysis') {
//              steps {
//                sh './scripts/run-static-analysis.py build-debug/'
//              }
//            }
//          }
//        } // clang 6 debug
//
//        stage('Clang 6 Release') {
//          agent {
//            docker {
//              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
//            }
//          }
//
//          stages {
//            stage('Release Build') {
//              steps {
//                sh './scripts/ci-tool.py -B Release'
//              }
//            }
//            stage('Unit Tests') {
//              steps {
//                sh './scripts/ci-tool.py -T Release'
//              }
//            }
//          }
//
//          post {
//            always {
//              junit 'build-release/TestResults.xml'
//            }
//          }
//        } // clang 6 release
//
//        stage('GCC 7 Debug') {
//          agent {
//            docker {
//              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
//            }
//          }
//
//          environment {
//            CC  = 'gcc'
//            CXX = 'g++'
//          }
//
//          stages {
//            stage('GCC Debug Build') {
//              steps {
//                sh './scripts/ci-tool.py -B Debug'
//              }
//            }
//            stage('GCC Debug Unit Tests') {
//              steps {
//                sh './scripts/ci-tool.py -T Debug'
//              }
//            }
//          }
//        } // gcc 7 debug
//
//        stage('GCC 7 Release') {
//          agent {
//            docker {
//              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
//            }
//          }
//
//          environment {
//            CC  = 'gcc'
//            CXX = 'g++'
//          }
//
//          stages {
//            stage('GCC Release Build') {
//              steps {
//                sh './scripts/ci-tool.py -B Release'
//              }
//            }
//            stage('GCC Release Unit Tests') {
//              steps {
//                sh './scripts/ci-tool.py -T Release'
//              }
//            }
//          }
//        } // gcc 7 release
//
//      } // parallel
//    } // build & test
//
//  } // stages

