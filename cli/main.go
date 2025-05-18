package main

import (
	"flag"
	"fmt"
	"os"
	"strings"

	git "github.com/go-git/go-git/v5"
	"github.com/go-git/go-git/v5/config"
	gitHttp "github.com/go-git/go-git/v5/plumbing/transport/http"
)

func main() {
	// Define the main arguments
	cloneCmd := flag.NewFlagSet("clone", flag.ExitOnError)
	addRemoteCmd := flag.NewFlagSet("addremote", flag.ExitOnError)
	pushCmd := flag.NewFlagSet("push", flag.ExitOnError)
	cloneAndPushCmd := flag.NewFlagSet("cloneandpush", flag.ExitOnError)

	// Define flags for the clone command
	directory := cloneCmd.String("directory", "", "Target directory for the clone")
	repo := cloneCmd.String("repo", "", "Repository URL to clone")
	isBare := cloneCmd.Bool("isbare", true, "Clone as a bare repository")
	username := cloneCmd.String("username", "", "Username for authentication")
	password := cloneCmd.String("password", "", "Password for authentication")

	// Define flags for the addremote command
	addRemoteRepo := addRemoteCmd.String("repo", "", "Repository URL to add as a remote")
	addRemoteDirectory := addRemoteCmd.String("directory", "", "Target directory for the repository")

	// Define flags for the push command
	pushDirectory := pushCmd.String("directory", "", "Target directory for the push")
	pushUsername := pushCmd.String("username", "", "Username for authentication")
	pushPassword := pushCmd.String("password", "", "Password for authentication")

	// Define flags for the cloneandpush command
	sourcerepo := cloneAndPushCmd.String("sourcerepo", "", "Source repository URL to clone")
	sourceusername := cloneAndPushCmd.String("sourceusername", "", "Source username for authentication")
	sourcepassword := cloneAndPushCmd.String("sourcepassword", "", "Source password for authentication")
	destrepo := cloneAndPushCmd.String("destrepo", "", "Destination repository URL to clone")
	destusername := cloneAndPushCmd.String("destusername", "", "Destination username for authentication")
	destpassword := cloneAndPushCmd.String("destpassword", "", "Destination password for authentication")
	hasWiki := cloneAndPushCmd.Bool("haswiki", false, "Set to true if the repository has a wiki (default: false)")

	if len(os.Args) < 2 {
		fmt.Println("Expected 'cloneandpush', 'clone', 'addremote', or 'push' subcommands")
		os.Exit(1)
	}

	switch os.Args[1] {
	case "clone":
		cloneCmd.Parse(os.Args[2:])
		if *repo == "" || *directory == "" {
			fmt.Println("Both --repo and --directory are required for the clone command")
			os.Exit(1)
		}
		cloneRepository(*repo, *directory, *isBare, *username, *password)

	case "addremote":
		addRemoteCmd.Parse(os.Args[2:])
		if *addRemoteRepo == "" || *addRemoteDirectory == "" {
			fmt.Println("Both --repo and --directory are required for the addremote command")
			os.Exit(1)
		}
		addRemote(*addRemoteDirectory, *addRemoteRepo)

	case "push":
		pushCmd.Parse(os.Args[2:])
		if *pushDirectory == "" {
			fmt.Println("--directory is required for the push command")
			os.Exit(1)
		}
		pushToRemote(*pushDirectory, *pushUsername, *pushPassword)

	case "cloneandpush":
		cloneAndPushCmd.Parse(os.Args[2:])
		if *sourcerepo == "" || *sourceusername == "" || *sourcepassword == "" || *destrepo == "" || *destusername == "" || *destpassword == "" {
			fmt.Println("Some arguments are missing for the cloneandpush command")
			os.Exit(1)
		}
		cloneAndPush(*sourcerepo, *sourceusername, *sourcepassword, *destrepo, *destusername, *destpassword, *hasWiki)

	default:
		fmt.Println("Unknown command. Expected 'clone', 'addremote', or 'push'")
		os.Exit(1)
	}
}

func cloneRepository(repo, directory string, isBare bool, username, password string) {
	var err error
	cloneOptions := &git.CloneOptions{
		URL: repo,
	}
	if username == "" {
		username = "git" // GitHub requires "git" as the username for PATs
	}
	if password != "" {
		cloneOptions.Auth = &gitHttp.BasicAuth{
			Username: username,
			Password: password,
		}
	}
	if isBare {
		_, err = git.PlainClone(directory, true, cloneOptions)
	} else {
		_, err = git.PlainClone(directory, false, cloneOptions)
	}
	if err != nil {
		fmt.Printf("Error cloning repository: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("Repository cloned successfully.")
}

func addRemote(directory, repo string) {
	r, err := git.PlainOpen(directory)
	if err != nil {
		fmt.Printf("Error opening repository: %v\n", err)
		os.Exit(1)
	}

	_, err = r.CreateRemote(&config.RemoteConfig{
		Name: "origin2",
		URLs: []string{repo},
	})
	if err != nil {
		fmt.Printf("Error adding remote: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("Remote added successfully.")
}

func pushToRemote(directory, username, password string) {
	r, err := git.PlainOpen(directory)
	if err != nil {
		fmt.Printf("Error opening repository: %v\n", err)
		os.Exit(1)
	}

	pushOptions := &git.PushOptions{
		RemoteName: "origin2",
		RefSpecs: []config.RefSpec{
			config.RefSpec("+refs/*:refs/*"),
		},
		Force: true,
	}
	if username == "" {
		username = "git" // GitHub requires "git" as the username for PATs
	}
	if password != "" {
		pushOptions.Auth = &gitHttp.BasicAuth{
			Username: username,
			Password: password,
		}
	}

	err = r.Push(pushOptions)
	if err != nil {
		fmt.Printf("Error pushing to remote: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("Pushed to remote successfully.")
}

func cloneAndPush(sourcerepo, sourceusername, sourcepassword, destrepo, destusername, destpassword string, hasWiki bool) {
	cnpdirectory := "temp.git"

	defer func() {
		err := os.RemoveAll(cnpdirectory)
		if err != nil {
			fmt.Printf("Warning: failed to remove temp directory %s: %v\n", cnpdirectory, err)
		}
	}()

	cloneRepository(sourcerepo, cnpdirectory, true, sourceusername, sourcepassword)
	addRemote(cnpdirectory, destrepo)
	pushToRemote(cnpdirectory, destusername, destpassword)

	if hasWiki {
		err := os.RemoveAll(cnpdirectory)
		if err != nil {
			fmt.Printf("Warning: failed to remove temp directory %s: %v\n", cnpdirectory, err)
		}

		sourceWikiUrl := strings.Replace(sourcerepo, ".git", ".wiki.git", 1)
		destWikiUrl := strings.Replace(destrepo, ".git", ".wiki.git", 1)
		cloneRepository(sourceWikiUrl, cnpdirectory, true, sourceusername, sourcepassword)
		addRemote(cnpdirectory, destWikiUrl)
		pushToRemote(cnpdirectory, destusername, destpassword)
	}
}
