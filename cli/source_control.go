package main

import (
	"fmt"
	"os"

	git "github.com/go-git/go-git/v5"
	"github.com/go-git/go-git/v5/config"
	gitHttp "github.com/go-git/go-git/v5/plumbing/transport/http"
)

// cloneRepository clones a repository to a local directory.
func cloneRepository(repo, path, username, password string) error {
	if username == "" {
		username = "git" // GitHub requires "git" as the username for PATs
	}

	r, err := git.PlainInit(path, true)
	if err != nil {
		return fmt.Errorf("error initializing repository: %w", err)
	}

	_, err = r.CreateRemote(&config.RemoteConfig{
		Name: "origin",
		URLs: []string{repo},
		Fetch: []config.RefSpec{
			"+refs/heads/*:refs/heads/*",
			"+refs/tags/*:refs/tags/*",
		},
	})
	if err != nil {
		return fmt.Errorf("error adding remote: %w", err)
	}

	var auth *gitHttp.BasicAuth
	if password != "" {
		auth = &gitHttp.BasicAuth{
			Username: username,
			Password: password,
		}
	}

	err = r.Fetch(&git.FetchOptions{
		RemoteName: "origin",
		RefSpecs: []config.RefSpec{
			"+refs/heads/*:refs/heads/*",
			"+refs/tags/*:refs/tags/*",
		},
		Auth: auth,
	})
	if err != nil {
		return fmt.Errorf("error fetching from remote: %w", err)
	}

	return nil
}

// addRemote adds a new remote to the local repository.
func addRemote(path, repo string) error {
	r, err := git.PlainOpen(path)
	if err != nil {
		return fmt.Errorf("error opening repository: %w", err)
	}

	_, err = r.CreateRemote(&config.RemoteConfig{
		Name: "origin2",
		URLs: []string{repo},
	})
	if err != nil {
		return fmt.Errorf("error adding remote: %w", err)
	}
	return nil
}

// pushToRemote pushes the local repository to the specified remote.
func pushToRemote(path, username, password string) error {
	r, err := git.PlainOpen(path)
	if err != nil {
		return fmt.Errorf("error opening repository: %w", err)
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
		return fmt.Errorf("error pushing to remote: %w", err)
	}
	return nil
}

// CloneAndPush clones a repository from the source URL and pushes it to the destination URL.
func CloneAndPush(sourceRepo, sourceUsername, sourcePassword, destRepo, destUsername, destPassword string) error {
	tempPath := "temp.git"

	defer func() {
		err := os.RemoveAll(tempPath)
		if err != nil {
			fmt.Printf("Warning: failed to remove temp directory %s: %v\n", tempPath, err)
		}
	}()

	if err := cloneRepository(sourceRepo, tempPath, sourceUsername, sourcePassword); err != nil {
		return fmt.Errorf("clone source repo: %w", err)
	}
	if err := addRemote(tempPath, destRepo); err != nil {
		return fmt.Errorf("add destination remote: %w", err)
	}
	if err := pushToRemote(tempPath, destUsername, destPassword); err != nil {
		return fmt.Errorf("push to destination remote: %w", err)
	}

	return nil
}
