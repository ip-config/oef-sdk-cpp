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


